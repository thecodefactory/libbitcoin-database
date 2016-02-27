/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/disk/mmfile.hpp>

#include <iostream>

#ifdef _WIN32
    #include <io.h>
    #include "../mman-win32/mman.h"
    #define FILE_OPEN_PERMISSIONS _S_IREAD | _S_IWRITE
#else
    #include <unistd.h>
    #include <stddef.h>
    #include <sys/mman.h>
    #define FILE_OPEN_PERMISSIONS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#endif
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <bitcoin/bitcoin.hpp>

// mmfile is be able to support 32 bit but because the database 
// requires a larger file this is not validated or supported.
static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

namespace libbitcoin {
namespace database {

using boost::format;
using boost::filesystem::path;

static constexpr size_t growth_factor = 3 / 2;

size_t mmfile::file_size(int file_handle)
{
    if (file_handle == -1)
        return 0;

    // This is required because off_t is defined as long, whcih is 32 bits in
    // msvc and 64 bits in linux/osx, and stat contains off_t.
#ifdef _WIN32
#ifdef _WIN64
    struct _stat64 sbuf;
    if (_fstat64(file_handle, &sbuf) == -1)
        return 0;
#else
    struct _stat32 sbuf;
    if (_fstat32(file_handle, &sbuf) == -1)
        return 0;
#endif
#else
    struct stat sbuf;
    if (fstat(file_handle, &sbuf) == -1)
        return 0;
#endif

    // Convert signed to unsigned size.
    BITCOIN_ASSERT_MSG(sbuf.st_size > 0, "File size cannot be 0 bytes.");
    return static_cast<size_t>(sbuf.st_size);
}

int mmfile::open_file(const path& filename)
{
#ifdef _WIN32
    int handle = _wopen(filename.wstring().c_str(), O_RDWR,
        FILE_OPEN_PERMISSIONS);
#else
    int handle = open(filename.string().c_str(), O_RDWR,
        FILE_OPEN_PERMISSIONS);
#endif
    return handle;
}

void mmfile::handle_error(const char* context, const path& filename)
{
    static const auto form = "The file failed to %1%: %2% error: %3%";
#ifdef _WIN32
    const auto error = GetLastError();
#else
    const auto error = errno;
#endif
    const auto message = format(form) % context % filename % error;
    log::fatal(LOG_DATABASE) << message.str();
}

mmfile::mmfile(const path& filename)
  : filename_(filename),
    file_handle_(open_file(filename_)),
    size_(file_size(file_handle_)),
    data_(nullptr),
    stopped_(false)
{
    // This initializes data_.
    stopped_ = !map(size_);

    if (stopped_)
        handle_error("map", filename_);
    else
        log::info(LOG_DATABASE) << "Mapping: " << filename_;
}

mmfile::~mmfile()
{
    stop();
}

bool mmfile::stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::unique_lock<boost::shared_mutex> unique_lock(mutex_);

    if (stopped_)
        return true;

    log::info(LOG_DATABASE) << "Unmapping: " << filename_;
    const auto unmapped = unmap();
    if (!unmapped)
        handle_error("unmap", filename_);

#ifdef _WIN32
    const auto handle = (HANDLE)_get_osfhandle(file_handle_);
    const auto flushed = FlushFileBuffers(handle) != FALSE;
#else
    // Calling fsync() does not necessarily ensure that the entry in the 
    // directory containing the file has also reached disk. For that an
    // explicit fsync() on a file descriptor for the directory is also needed.
    const auto flushed = fsync(file_handle_) != -1;
#endif
    if (!flushed)
        handle_error("flush", filename_);

    const auto closed = close(file_handle_) != -1;
    if (!closed)
        handle_error("close", filename_);

    stopped_ = true;
    return unmapped && flushed && closed;
    ///////////////////////////////////////////////////////////////////////////
}

// This is thread safe but only useful on initialization.
size_t mmfile::size() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::shared_lock<boost::shared_mutex> shared_lock(mutex_);

    return size_;
    ///////////////////////////////////////////////////////////////////////////
}

// There is no guard against calling when stopped.
void mmfile::resize(size_t size)
{
    // This establishes a shared lock during this one line.
    /* write_accessor */ writer(size);
}

// There is no guard against calling when stopped.
const read_accessor mmfile::reader() const
{
    // This establishes a shared lock until disposed.
    return read_accessor(data_, mutex_);
}

// There is no guard against calling when stopped.
write_accessor mmfile::writer(size_t size)
{
    // This establishes an upgradeable shared lock until disposed.
    write_accessor accessor(data_, mutex_);

    if (size > size_)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        write_accessor::upgrade unique_lock(accessor.get_upgradeable());

        // Must retest under the unique lock.
        if (size > size_)
        {
            // There is no way to recover from this.
            if (!reserve(size))
                throw std::runtime_error(
                    "The file could not be resized, disk space may be low.");
        }
        ///////////////////////////////////////////////////////////////////////
    }

    return accessor;
}

// privates

// This sets data_ and size_, used on construct and resize.
bool mmfile::map(size_t size)
{
    if (size == 0)
        return false;

    data_ = static_cast<uint8_t*>(mmap(0, size, PROT_READ | PROT_WRITE,
        MAP_SHARED, file_handle_, 0));

    return validate(size);
}

#ifdef MREMAP_MAYMOVE
bool mmfile::remap(size_t new_size)
{
    data_ = static_cast<uint8_t*>(mremap(data_, size_, new_size,
        MREMAP_MAYMOVE));

    return validate(new_size);
}
#endif

bool mmfile::unmap()
{
    bool success = (munmap(data_, size_) != -1);
    size_ = 0;
    data_ = nullptr;
    return success;
}

bool mmfile::reserve(size_t size)
{
    const size_t new_size = size * growth_factor;

    // Resize underlying file.
    if (ftruncate(file_handle_, new_size) == -1)
    {
        handle_error("resize", filename_);
        return false;
    }

    const auto message = format("Resizing: %1% [%2%]") % filename_ % new_size;
    log::debug(LOG_DATABASE) << message.str();

    // Readjust memory map.
#ifdef MREMAP_MAYMOVE
    return remap(new_size);
#else
    return (unmap() && map(new_size));
#endif
}

bool mmfile::validate(size_t size)
{
    if (data_ == MAP_FAILED)
    {
        size_ = 0;
        data_ = nullptr;
        return false;
    }

    size_ = size;
    return true;
}

} // namespace database
} // namespace libbitcoin

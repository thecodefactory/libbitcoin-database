/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_SLAB_ROW_HPP
#define LIBBITCOIN_DATABASE_SLAB_ROW_HPP

#include <cstddef>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>

namespace libbitcoin {
namespace database {

/**
 * Item for slab_hash_table. A chained list with the key included.
 *
 * Stores the key, next position and user data.
 * With the starting item, we can iterate until the end using the
 * next_position() method.
 */
template <typename KeyType>
class slab_row
{
public:
    typedef byte_serializer::functor write_function;
    static const file_offset not_found = bc::max_uint64;
    static const size_t key_start = 0;
    static const size_t key_size = std::tuple_size<KeyType>::value;
    static const size_t position_size = sizeof(file_offset);
    static const file_offset prefix_size = key_size + position_size;

    // Construct for a new slab.
    slab_row(slab_manager& manager);

    // Construct for an existing slab.
    slab_row(slab_manager& manager, file_offset position);

    /// Allocate and populate a new slab.
    file_offset create(const KeyType& key, write_function write,
        size_t value_size);

    /// Link allocated/populated slab.
    void link(file_offset next);

    /// Does this match?
    bool compare(const KeyType& key) const;

    /// The actual user data.
    memory_ptr data() const;

    /// The file offset of the user data.
    file_offset offset() const;

    /// Position of next slab in the list.
    file_offset next_position() const;

    /// Write the next position.
    void write_next_position(file_offset next);

private:
    memory_ptr raw_data(file_offset offset) const;

    file_offset position_;
    slab_manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/slab_row.ipp>

#endif

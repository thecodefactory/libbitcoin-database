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
#ifndef LIBBITCOIN_DATABASE_ITERABLE_HPP
#define LIBBITCOIN_DATABASE_ITERABLE_HPP

#include <bitcoin/database/primitives/iterator.hpp>

namespace libbitcoin {
namespace database {

template <typename Manager, typename Link>
class iterable
{
public:
    iterable(const Manager& manager, Link begin);

    bool empty() const;
    Link front() const;
    iterator<Manager, Link> begin() const;
    iterator<Manager, Link> end() const;

private:
    Link begin_;
    const Manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/iterable.ipp>

#endif

/*
 *
 * Copyright (C) 2023-2024 Patryk Jaworski (blog.regalis.tech)
 *
 * Author: Patryk Jaworski <regalis@regalis.tech>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef HD44780_COMMON_HPP
#define HD44780_COMMON_HPP

#include "instructions.hpp"

namespace drivers::lcd::hd44780 {
struct configuration
{
    uint8_t columns = 16;
    uint8_t lines = 2;
    font font_size = font::font_5x8;
};

enum class alignment
{
    no_alignment,
    left,
    center,
    right,
};

enum class line : uint8_t
{
    line_1 = 0,
    line_2,
    line_3,
    line_4,
};

namespace detail {

template<auto T>
struct interface_for
{
    static_assert(false,
                  "Undefined interface for the specified descriptor. This is "
                  "probably a driver bug.");
    using type = std::false_type;
};
}

template<auto T>
using interface_for = detail::interface_for<T>::type;
}

#endif

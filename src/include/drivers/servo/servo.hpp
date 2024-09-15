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

#ifndef DRIVERS_SERVO_HPP
#define DRIVERS_SERVO_HPP

#include "interface_helpers.hpp"

namespace drivers::servo {

template<typename Interface, class... Features>
class servo : Features...
{
  public:
    using interface = Interface;

    constexpr void init() const
    {
        interface::init_mcu_interface();
        interface::init_servo_interface();
    }

    // TODO: use strong types for the angle!
    constexpr void set_angle(uint32_t angle) const
    {
        interface::set_angle(angle);
    }

    constexpr void enable() const
    {
        interface::enable();
    }

    constexpr void disable() const
    {
        interface::disable();
    }
};

}

#endif

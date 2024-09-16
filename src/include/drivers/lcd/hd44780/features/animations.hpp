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

#ifndef HD44780_FEATURES_WITH_ANIMATIONS
#define HD44780_FEATURES_WITH_ANIMATIONS

#include <algorithm>
#include <chrono>
#include <expected>
#include <string_view>

#include "../common.hpp"
#include "../instructions.hpp"
#include "timer.hpp"

namespace drivers::lcd::hd44780::features {

namespace detail {
template<uint32_t CharactersDelayMilliseconds>
struct with_animations
{
    template<typename Self>
    constexpr uint32_t animate_puts(this Self&& self, std::string_view str)
    {
        std::ranges::for_each(str, [self](const char character) {
            auto _ = timer::scoped_delay{std::chrono::milliseconds{CharactersDelayMilliseconds}};
            self.putc(character);
        });

        return str.length();
    }

    template<typename Self>
    constexpr uint32_t animate_puts(this Self&& self,
                                    std::string_view str,
                                    line line_number,
                                    alignment align = alignment::no_alignment)
    {
        self.prepare_cursor_position(str, line_number, align);
        return self.animate_puts(str);
    }

    /**
     * Clean specified number of characters with a nice looking animation.
     *
     * WARNING! This function with leave the cursor **behind the last removed character**, use
     * `cursor_goto()` to set the cursor into the desired position.
     *
     * TODO: check if LCD supports reading the position of a cursor and leave the cursor in the
     * expected position.
     */
    template<typename Self>
    constexpr uint32_t animate_clear(this Self&& self, uint32_t number_of_characters)
    {
        using Interface = typename std::decay_t<Self>::interface;

        Interface::send_instruction(instructions::entry_mode_set(mode::decrement, shift::off));

        while (number_of_characters != 0) {
            auto _ = timer::scoped_delay{std::chrono::milliseconds{CharactersDelayMilliseconds}};
            self.putc(' ');
            number_of_characters = number_of_characters - 1;
        }
        self.putc(' ');

        Interface::send_instruction(instructions::entry_mode_set(mode::increment, shift::off));

        return number_of_characters;
    }

    constexpr uint32_t animate_clear_line(this auto&& self, line line_no)
    {
        self.cursor_goto(self.config.columns - 1, line_no);
        auto result = self.animate_clear(self.config.columns);
        self.cursor_goto(0, line_no);
        return result;
    }
};
}

using with_animations = detail::with_animations<50>;

template<uint32_t CharactersDelayMilliseconds>
using with_animations_custom_delay = detail::with_animations<CharactersDelayMilliseconds>;

// TODO: update concept to cover all functions
template<typename T>
concept has_animations = requires(T lcd) {
    { lcd.animate_puts("Hello world") };
    { lcd.animate_clear(10) };
};

}

#endif

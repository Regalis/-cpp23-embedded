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

#ifndef HD44780_HPP
#define HD44780_HPP

#include <algorithm>
#include <chrono>
#include <expected>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

#include "common.hpp"

namespace drivers::lcd::hd44780 {

template<typename Interface, configuration Config, class... Features>
class hd44780 : public Features...
{
  public:
    using interface = Interface;
    static constexpr configuration config = Config;

    constexpr void init() const
    {
        using namespace std::chrono_literals;
        using namespace instructions;

        interface::init_mcu_interface();
        interface::init_lcd_interface();

        constexpr lines number_of_lines = (Config.lines > 1 ? lines::two_lines : lines::one_line);

        interface::send_instruction(
          instructions::function_set(interface::lcd_interface, number_of_lines, config.font_size));

        interface::delay(1ms);

        interface::send_instruction(
          instructions::display_on_off(power::off, cursor::off, blink::off));

        interface::delay(1ms);
        interface::send_instruction(instructions::clear_display());
        interface::delay(2ms);

        interface::send_instruction(instructions::entry_mode_set(mode::increment, shift::off));

        interface::delay(1ms);

        interface::send_instruction(
          instructions::display_on_off(power::on, cursor::off, blink::off));
        interface::delay(1ms);

        init_features();
    }

    /**
     * Clear display and return home (x = 0, y = 0)
     */
    constexpr void clear() const
    {
        interface::send_instruction(instructions::clear_display());
        home();
    }

    constexpr void home() const
    {
        using namespace std::chrono_literals;
        interface::send_instruction(instructions::return_home());
        interface::delay(2ms);
    }

    constexpr void cursor_goto(uint8_t x, uint8_t y) const
    {
        uint8_t addr = x & 0x3f;
        if (y == 1 || y == 3) {
            addr += 0x40;
        }
        if (y == 2 || y == 3) {
            addr += config.columns;
        }
        interface::send_instruction(instructions::ddram_set(addr));
    }

    constexpr void cursor_goto(uint8_t x, line line_no) const
    {
        cursor_goto(x, std::to_underlying(line_no));
    }

    constexpr void display_on(cursor cursor = cursor::off, blink blink = blink::off) const
    {
        interface::send_instruction(instructions::display_on_off(power::on, cursor, blink));
    }

    constexpr void display_off() const
    {
        interface::send_instruction(power::off, cursor::off, blink::off);
    }

    constexpr void clear_line(uint8_t line) const
    {
        cursor_goto(0, line);
        constexpr uint8_t start = 0;
        constexpr uint8_t stop = config.columns;
        std::ranges::for_each(std::views::iota(start, stop), [this](auto) {
            putc(' ');
        });
        cursor_goto(0, line);
    }

    constexpr void clear_line(line line_no) const
    {
        clear_line(std::to_underlying(line_no));
    }

    constexpr void putc(char character) const
    {
        interface::send_data(static_cast<uint8_t>(character));
    }

    constexpr uint32_t puts(std::string_view str) const
    {
        for (auto character : str) {
            putc(character);
        }
        return str.length();
    }

    constexpr uint32_t puts(std::string_view str,
                            line line_no,
                            alignment align = alignment::no_alignment)
    {
        prepare_cursor_position(str, line_no, align);
        return puts(str);
    }

    constexpr std::optional<std::tuple<uint8_t, uint8_t>>
      prepare_cursor_position(std::string_view str, line line_no, alignment align) const
    {
        std::tuple<uint8_t, uint8_t> result{0, std::to_underlying(line_no)};
        auto& [x, y] = result;
        switch (align) {
            case alignment::no_alignment:
                return std::nullopt;
            case alignment::left:
                x = 0;
                break;
            case alignment::center: {
                if (str.length() >= config.columns) {
                    x = 0;
                } else {
                    int position =
                      (((config.columns / 2)) - (static_cast<int8_t>(str.length()) / 2));
                    x = static_cast<uint8_t>((position < 0x00 ? 0x00 : position));
                }
                break;
            }
            case alignment::right: {
                if (str.length() >= config.columns) {
                    x = 0;
                } else {
                    x = static_cast<uint8_t>(config.columns - str.length());
                }
                break;
            }
        }

        cursor_goto(x, y);
        return result;
    }

  protected:
    constexpr void init_features(this auto&& self)
    {
        using enabled_features = std::tuple<Features...>;

        auto initialize_feature = [self]<std::size_t I>(std::integral_constant<std::size_t, I>) {
            using feature_t = std::tuple_element_t<I, enabled_features>;
            if constexpr (requires(feature_t f) { f.init(); }) {
                self.feature_t::init();
            }
        };

        [=]<std::size_t... I>(std::index_sequence<I...>) {
            ((initialize_feature(std::integral_constant<std::size_t, I>()), ...));
        }(std::make_index_sequence<std::tuple_size_v<enabled_features>>());
    }
};

}

#endif

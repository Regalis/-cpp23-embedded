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

#ifndef HD44780_FEATURES_WITH_CUSTOM_CHARACTERS
#define HD44780_FEATURES_WITH_CUSTOM_CHARACTERS

#include <bitset>
#include <expected>
#include <ranges>
#include <string_view>

#include "../instructions.hpp"
#include "timer.hpp"

namespace drivers::lcd::hd44780::features {

namespace detail {
template<uint8_t Width, uint8_t Height>
class lcd_custom_char
{
  public:
    static constexpr auto width = Width;
    static constexpr auto height = Height;

    constexpr lcd_custom_char(std::string_view char_definition)
    {
        auto font = char_definition | std::views::split(std::string_view{"\n"}) |
                    std::views::filter([](auto line) {
                        return line.size() > 0;
                    });

        for (const auto& [index, line] : font | std::views::enumerate) {
            std::bitset<16> bits;
            for (int i = 0; i < Width; ++i) {
                if (line[i] == ' ') {
                    continue;
                }
                bits.set(Width - i - 1);
            }
            m_data[index] = bits.to_ulong();
        }
    }

    constexpr bool operator==(const lcd_custom_char<Width, Height>&) const = default;

  public:
    std::array<uint16_t, Height> m_data;
};

}

namespace custom_character_literals {
consteval auto operator""_lcd_char_5x8(const char* data, std::size_t length)
{
    return detail::lcd_custom_char<5, 8>{std::string_view{data, data + length}};
}
}

template<uint8_t Width, uint8_t Height, detail::lcd_custom_char<Width, Height>... Characters>
struct custom_characters_set
{
    static constexpr std::array custom_characters{Characters...};
    using lcd_char_t = detail::lcd_custom_char<Width, Height>;

    struct not_found
    {};

    static constexpr auto character_position(const detail::lcd_custom_char<Width, Height> c)
      -> std::expected<uint8_t, not_found>
    {
        for (const auto& [index, defined_char] : custom_characters | std::views::enumerate) {
            if (defined_char == c) {
                return {static_cast<uint8_t>(index)};
            }
        }
        return std::unexpected(not_found{});
    }
};

template<class CharacterSet>
struct with_custom_characters
{
    using character_set_t = CharacterSet;

    constexpr void put_custom_character(this auto&& self,
                                        const typename character_set_t::lcd_char_t& c)
    {
        auto position = character_set_t::character_position(c);
        if (position) {
            self.putc(position.value());
        }

        // TODO: character not found! This should be a compile-time error!
    }

    template<typename Self>
    constexpr void init(this Self&&)
    {
        using interface = std::decay_t<Self>::interface;
        for (const auto& [character_index, character_descriptor] :
             character_set_t::custom_characters | std::views::enumerate) {

            for (const auto& [glyph_index, glyph_row] :
                 character_descriptor.m_data | std::views::enumerate) {
                // TODO: FIXME: this calculations work only for 5x8 font!
                // We need to check the configured font and act accordingly
                uint8_t cgram_address = (static_cast<uint8_t>(character_index & 0x7) << 3) |
                                        (static_cast<uint8_t>(glyph_index) & 0x1F);

                interface::send_instruction(hd44780::instructions::cgram_set(cgram_address));
                interface::send_data(glyph_row);
            }
        }
    }
};

}

#endif

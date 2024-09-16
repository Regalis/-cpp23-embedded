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

#include "clocks.hpp"
#include "drivers/lcd/hd44780/features.hpp"
#include "drivers/lcd/hd44780/hd44780.hpp"
#include "drivers/lcd/hd44780/interfaces/gpio4_bit.hpp"
#include "gpio.hpp"
#include "reset.hpp"
#include "rp2040.hpp"
#include "timer.hpp"

#include <algorithm>
#include <chrono>
#include <string_view>

using namespace std::chrono_literals;
using namespace std::string_view_literals;
using namespace drivers::lcd;
using namespace hd44780::features;

using namespace hd44780::features::custom_character_literals;

constexpr auto battery_empty = R"(
 ### 
#   #
#   #
#   #
#   #
#   #
#####
     
)"_lcd_char_5x8;

constexpr auto battery_low = R"(
 ### 
#   #
#   #
#   #
#   #
#  ##
#####
     
)"_lcd_char_5x8;

constexpr auto battery_medium = R"(
 ### 
#   #
#   #
#  ##
# ###
#####
#####
     
)"_lcd_char_5x8;

constexpr auto battery_almost_full = R"(
 ### 
#  ##
# ###
#####
#####
#####
#####
     
)"_lcd_char_5x8;

constexpr auto battery_full = R"(
 ### 
#####
#####
#####
#####
#####
#####
     
)"_lcd_char_5x8;

int main()
{
    // We need watchdog to use timer::delay()
    clocks::init();
    clocks::watchdog_start(platform::xosc::frequency_khz);

    // Every peripheral is held in reset at power-up
    // We need to release io_bank0 from the reset state to be able to use GPIOs
    reset::release_subsystem_wait(reset::subsystems::io_bank0);

    // For PWM (LCD backlight control)
    reset::release_subsystem_wait(reset::subsystems::pwm);

    gpio::pin<platform::pins::gpio25> led0;
    led0.function_select(gpio::functions::sio);
    led0.set_as_output();

    // Prepere descriptor for the selected interface
    constexpr auto descriptor =
      hd44780::interfaces::gpio4_bit{.register_select = platform::pins::gpio10,
                                     .enable = platform::pins::gpio11,
                                     .data4 = platform::pins::gpio12,
                                     .data5 = platform::pins::gpio13,
                                     .data6 = platform::pins::gpio14,
                                     .data7 = platform::pins::gpio15};

    // Define your LCD hardware layout
    constexpr auto configuration =
      hd44780::configuration{.columns = 20, .lines = 4, .font_size = hd44780::font::font_5x8};

    using battery_icons = custom_characters_set<5,
                                                8,
                                                battery_empty,
                                                battery_low,
                                                battery_medium,
                                                battery_almost_full,
                                                battery_full>;

    // Get the type of the driver based on your descriptor and configuration
    using lcd_t = hd44780::hd44780<hd44780::interface_for<descriptor>,
                                   configuration,
                                   with_animations,
                                   with_backlight_control_via_pwm<platform::pins::gpio18>,
                                   with_custom_characters<battery_icons>>;

    // Main
    constexpr lcd_t lcd{};

    // Initialize both the MCU interface (in this case - the GPIOs) and the LCD itself
    lcd.init();

    timer::delay(500ms);

    while (true) {
        led0.toggle();

        // clear the display
        lcd.clear();

        timer::delay(500ms);

        // smooth print
        lcd.animate_puts("Hello world");

        // go to the second line
        lcd.cursor_goto(0, 1);
        // smooth print, save number of printed characters
        auto length = lcd.animate_puts("blog.regalis.tech");

        timer::delay(1s);

        // smooth clear, use the previously saved length
        lcd.animate_clear(length);

        timer::delay(1s);

        // back to the second line
        lcd.cursor_goto(0, 1);
        lcd.animate_puts("Happy hacking :)");

        timer::delay(1s);

        // smoothly change the brightness to 5%
        lcd.backlight_fade_into(5);

        timer::delay(1s);

        // smoothly change the brightness to 100%
        lcd.backlight_fade_into(100);

        timer::delay(1s);

        lcd.cursor_goto(0, 3);
        length = lcd.animate_puts("Custom character: ");

        const auto show_custom_character = [&](const auto& character) {
            auto _ = timer::scoped_delay(500ms);
            lcd.cursor_goto(static_cast<uint8_t>(length), 3);
            lcd.put_custom_character(character);
        };

        std::ranges::for_each(battery_icons::custom_characters, show_custom_character);
        std::ranges::for_each(battery_icons::custom_characters | std::views::reverse,
                              show_custom_character);

        timer::delay(5s);
    }
}

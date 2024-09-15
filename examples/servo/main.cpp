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
#include "drivers/servo/interfaces/pwm.hpp"
#include "drivers/servo/servo.hpp"
#include "gpio.hpp"
#include "reset.hpp"
#include "rp2040.hpp"
#include "timer.hpp"

#include <algorithm>
#include <ranges>

using namespace std::chrono_literals;
using namespace drivers;

int main()
{
    clocks::init();
    clocks::watchdog_start(platform::xosc::frequency_khz);

    // For GPIO
    reset::release_subsystem_wait(reset::subsystems::io_bank0);
    // For PWM
    reset::release_subsystem_wait(reset::subsystems::pwm);

    gpio::pin<platform::pins::gpio25> led0;
    led0.function_select(gpio::functions::sio);
    led0.set_as_output();

    constexpr auto descriptor = servo::interfaces::pwm::on_pin(platform::pins::gpio17);

    using servo_t = drivers::servo::servo<servo::interface_for<descriptor>>;

    constexpr servo_t my_servo{};

    // Initialize MCU peripherals
    my_servo.init();

    // Set starting point
    my_servo.set_angle(0);

    // Enable signal generation
    my_servo.enable();

    constexpr std::array servo_positions{0, 45, 90, 135, 180};

    while (true) {
        led0.toggle();
        timer::delay(1s);
        std::ranges::for_each(servo_positions, [&](auto angle) {
            led0.toggle();
            my_servo.set_angle(angle);
            timer::delay(3s);
        });
        timer::delay(1s);
    }
}

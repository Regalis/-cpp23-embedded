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

#ifndef HD44780_FEATURES_WITH_BACKLIGHT_CONTROL_PWM
#define HD44780_FEATURES_WITH_BACKLIGHT_CONTROL_PWM

#include <algorithm>
#include <chrono>
#include <expected>
#include <ranges>

#include "../instructions.hpp"
#include "gpio.hpp"
#include "pwm.hpp"
#include "timer.hpp"
#include "utils.hpp"

namespace drivers::lcd::hd44780::features {

constexpr static auto default_frequency_config [[maybe_unused]] =
  pwm::get_frequency_config_for(120);

// TODO: use strong type for brightness levels (percentage)
template<platform::pins GPIO_PIN,
         uint8_t InitialBrightnessPercent = 100,
         pwm::frequency_config PWMFrequencyConfig = default_frequency_config>
struct with_backlight_control_pwm
{
    constexpr static auto gpio_pin = gpio::pin<GPIO_PIN>{};
    constexpr static auto pwm_slice = pwm::from_gpio(gpio_pin);
    using pwm_channel = pwm::channel_for_pin<GPIO_PIN>;

    // 0% duty cycle
    constexpr static auto pwm_min_channel_value = 0;

    // 100% duty cycle = counter wrap value
    constexpr static auto pwm_max_channel_value = PWMFrequencyConfig.wrap;

    constexpr void init(this auto&& self)
    {
        // Change PIN function to PWM
        self.gpio_pin.function_select(gpio::functions::pwm);

        // Set frequency and wrap values
        self.pwm_slice.set_frequency(PWMFrequencyConfig);

        // Set the initial brightness level
        self.backlight_set_brightness(InitialBrightnessPercent);

        // Enable signal generation
        self.pwm_slice.enable();
    }

    constexpr void backlight_set_brightness(this auto&& self, uint32_t brightness_percent)
    {
        const auto channel_level = percent_to_pwm_channel_level(brightness_percent);
        self.pwm_slice.set_channel_levels(pwm_channel{channel_level});
    }

    constexpr void backlight_fade_into(this auto&& self, uint32_t target_brightness_percent)
    {
        const uint32_t current_brigtness =
          pwm_channel_level_to_percent(self.pwm_slice.get_channel_level(pwm_channel{}));
        int32_t step = (current_brigtness > target_brightness_percent ? -1 : 1);
        for (uint32_t brightness = current_brigtness; brightness != target_brightness_percent;
             brightness += step) {
            // TODO: animation speed should be configurable!
            auto _ = timer::scoped_delay{std::chrono::milliseconds(20)};
            self.backlight_set_brightness(brightness);
        }
    }

  protected:
    constexpr static uint16_t percent_to_pwm_channel_level(uint32_t percent)
    {
        return static_cast<uint16_t>(utils::map(
          static_cast<long int>(percent), 0, 100, pwm_min_channel_value, pwm_max_channel_value));
    }

    constexpr static uint32_t pwm_channel_level_to_percent(uint32_t channel_level)
    {
        return utils::map(channel_level, pwm_min_channel_value, pwm_max_channel_value, 0, 100);
    }
};

template<typename T>
concept has_backlight_control_pwm = requires(T lcd) {
    { lcd.backlight_set_brightness(10) };
};

}

#endif

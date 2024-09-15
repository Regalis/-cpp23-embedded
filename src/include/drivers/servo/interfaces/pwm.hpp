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

#ifndef DRIVERS_SERVO_INTERFACES_PWM
#define DRIVERS_SERVO_INTERFACES_PWM

#include <chrono>

#include "../../../pwm.hpp"
#include "../interface_helpers.hpp"
#include "gpio.hpp"
#include "rp2040.hpp"
#include "utils.hpp"

namespace drivers::servo::interfaces::detail {

struct pwm_descriptor
{
    platform::pins pin;

    struct
    {
        // TODO: use strong types (chrono::microseconds)!
        uint32_t frequency_hz{50};
        uint32_t pulse_width_for_0_degrees_in_us{800};
        uint32_t pulse_width_for_180_degrees_in_us{2500};
    } timing_configuration;

    constexpr static pwm_descriptor on_pin(platform::pins requested_pin)
    {
        return pwm_descriptor{.pin = requested_pin,
                              .timing_configuration =
                                decltype(pwm_descriptor::timing_configuration){}};
    }
};

template<pwm_descriptor InterfaceDescriptor>
class pwm
{
  public:
    static constexpr pwm_descriptor descriptor = InterfaceDescriptor;

    static constexpr void init_mcu_interface()
    {
        // Change the pin function
        servo_gpio_pin.function_select(gpio::functions::pwm);

        // Apply PWM configuration
        pwm_slice.set_frequency(pwm_frequency_config);
    }

    static constexpr void init_servo_interface()
    {
        // no need here
    }

    // TODO: use strong types for the angle!
    static constexpr void set_angle(uint32_t angle)
    {
        const auto target_channel_value = utils::map(static_cast<long int>(angle),
                                                     0,
                                                     180,
                                                     servo_channel_value_for_0_deg,
                                                     servo_channel_value_for_180_deg);
        pwm_slice.set_channel_levels(pwm_channel{static_cast<uint16_t>(target_channel_value)});
    }

    static constexpr uint32_t get_angle()
    {
        // TODO
        return 0;
    }

    static constexpr void enable()
    {
        pwm_slice.enable();
    }

    static constexpr void disable()
    {
        pwm_slice.disable();
    }

  protected:
    //
    // Compile time calculations!
    //
    static constexpr auto servo_gpio_pin = gpio::pin<descriptor.pin>{};
    static constexpr auto pwm_slice = ::pwm::from_gpio(servo_gpio_pin);
    static constexpr auto pwm_frequency_config =
      ::pwm::get_frequency_config_for(descriptor.timing_configuration.frequency_hz);

    static constexpr uint32_t full_duty_cycle_us =
      (std::chrono::microseconds{std::chrono::seconds{1}} /
       descriptor.timing_configuration.frequency_hz)
        .count();

    // 100% duty cycle = counter wrap value
    // TODO: verify datasheet!
    static constexpr uint32_t max_channel_value = pwm_frequency_config.wrap;
    static constexpr uint32_t min_channel_value = 0;

    static constexpr auto servo_channel_value_for_0_deg =
      utils::map(descriptor.timing_configuration.pulse_width_for_0_degrees_in_us,
                 0,
                 full_duty_cycle_us,
                 0,
                 max_channel_value);

    static constexpr auto servo_channel_value_for_180_deg =
      utils::map(descriptor.timing_configuration.pulse_width_for_180_degrees_in_us,
                 0,
                 full_duty_cycle_us,
                 0,
                 max_channel_value);

    using pwm_channel = ::pwm::channel_for_pin<descriptor.pin>;
};

}

namespace drivers::servo::interfaces {
using pwm = detail::pwm_descriptor;

}

namespace drivers::servo::detail {

template<interfaces::pwm descriptor>
struct interface_for<descriptor>
{
    using type = interfaces::detail::pwm<descriptor>;
};

}

#endif

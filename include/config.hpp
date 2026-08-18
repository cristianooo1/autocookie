#pragma once

#include <array>
#include <type_traits>

/*
https://www.learncpp.com/cpp-tutorial/scoped-enumerations-enum-classes/

Overload the unary + operator to convert an enum to the underlying type
*/
template <typename T>
constexpr auto operator+(T a) noexcept
{
    return static_cast<std::underlying_type_t<T>>(a);
}

namespace Config
{
    enum class RewardMode
    {
        TimeSuccess,
        TimeSuccessLogPotential,
        OriginalCookiesPlusCps,
    };
    // MODIFY THIS TO CHANGE REWARD MODE !!!!!!!!!!!!!!!!
    constexpr RewardMode reward_mode{RewardMode::TimeSuccessLogPotential};

    constexpr double episode_length{10000.0};
    constexpr double target_cookies{1000000.0};

    constexpr double progress_shaping_beta{0.1};

    // these are the geometrics sums
    // 1 + 1.15 + 1.15² + ... + 1.15⁹
    constexpr double building_price_multiplier_buy_10{20.303718238};

    // 1 + 1.15 + 1.15² + ... + 1.15⁹⁹
    constexpr double building_price_multiplier_buy_100{7828749.671335256};

    constexpr double plastic_mouse_unlock_cookies{1000.0};

    constexpr double building_price_growth{1.15};
    constexpr double buying_time_cost{0.2};

    constexpr double clicking_frequency{5.0};

    constexpr double golden_cookie_min_time{300.0};
    constexpr double golden_cookie_max_time{900.0};
    constexpr double golden_cookie_checks_per_second{30.0};

};
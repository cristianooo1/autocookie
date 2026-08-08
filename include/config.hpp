#pragma once

#include <array>

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
    constexpr double building_price_multiplier_buy_10{20.303718238};
    constexpr double building_price_multiplier_buy_100{7828749.671335256};

    constexpr double buying_time_cost{0.2};

    constexpr double clicking_frequency{5.0};

    constexpr double episode_length{100.0};

};
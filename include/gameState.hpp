#pragma once

#include <array>
#include <vector>

#include "types.hpp"
#include "config.hpp"

#include "eventTypes.hpp"

struct GameState
{
    double current_simulation_time{0.0};

    double current_cookies{0.0};
    double alltime_cookies{0.0};
    double total_cps{0.0};
    double cookies_per_click{1.0};

    /*
    0 -> Cursor
    1 -> Grandma
    2 -> Farm
    3 -> Mine
    4 -> Factory
    */
    std::array<int, +BuildingType::BUILDING_COUNT> buildingsOwned{};

    std::vector<ActiveGoldenCookieBuff> activeGoldenCookieBuffs{};

    // upgrades_owned
};
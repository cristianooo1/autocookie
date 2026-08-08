#pragma once

#include <array>

#include "types.hpp"
#include "config.hpp"

struct GameState
{
    double current_time{0.0};

    double current_cookies{0.0};
    double alltime_cookies{0.0};
    double cps{0.0};
    double building_cps{0.0};
    double clicking_cps{0.0};

    double cookies_per_click{5.0};

    /*
    0 -> cursor
    1 -> grandma
    2 -> farm
    */
    std::array<int, +BuildingType::BUILDING_COUNT> buildingsOwned{
        0, // number of CURSORS owned
        0, // number of GRANDMAS owned
        0, // number of FARMS owned
    };

    // active_buffs with timers
    // upgrades_owned
};
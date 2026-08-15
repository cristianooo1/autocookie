#include "economySystem.hpp"
#include "eventTypes.hpp"

#include <algorithm>
#include <stdexcept>

double EconomySystem::calculateEffectiveCPS(
    const GameState &state,
    bool is_clicking_active)
{
    /*
    https://cookieclicker.fandom.com/wiki/Cookies_per_Second:
    base_cps = bulding_cps + clicking_cps
    effective_cps = base_cps * MULTIPLIERS (= upgrades, buffs)
    */

    double building_cps = 0.0;
    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        building_cps += static_cast<double>(state.buildingsOwned[i]) *
                        buildingsDefinitions[i].base_cps;
        // * upgrades for each building
    };

    double clicking_cps = 0.0;
    if (is_clicking_active)
    {
        clicking_cps = Config::clicking_frequency * state.cookies_per_click;
    }

    double clicking_multiplier = 1.0;
    double global_multiplier = 1.0;
    for (int i = 0; i < state.activeGoldenCookieBuffs.size(); i++)
    {
        if (state.activeGoldenCookieBuffs[i].buff_type == GoldenCookieBuff::FRENZY)
        {
            global_multiplier *= 7.0;
        }
        if (state.activeGoldenCookieBuffs[i].buff_type == GoldenCookieBuff::CLICK_FRENZY)
        {
            clicking_multiplier *= 777.0;
        }
    }

    double effective_cps = (building_cps + clicking_cps * clicking_multiplier) * global_multiplier;
    return effective_cps;
}

void EconomySystem::integrateOverDT(
    GameState &state,
    const double dt,
    const bool is_clicking_active)
{
    constexpr double time_epsilon = 1e-9;

    // prevent negative dt
    if (dt < -time_epsilon)
    {
        throw std::logic_error("NEGATIVE DT ???????????????????");
    }

    double positive_dt = std::max(0.0, dt);

    double rate = this->calculateEffectiveCPS(state, is_clicking_active);

    state.current_cookies += rate * positive_dt;
    state.alltime_cookies += rate * positive_dt;
    state.total_cps = rate;
}

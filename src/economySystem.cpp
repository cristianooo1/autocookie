#include "economySystem.hpp"

void Economy::update(GameState &state, const double dt)
{
    /*
    https://cookieclicker.fandom.com/wiki/Cookies_per_Second:
    "To calculate the total CpS, first add together the CpS values for all buildings [...]
    This is the base CpS.
    The base CpS is then multiplied by several multipliers, one after the other."
    */

    double base_cps = Config::clicking_frequency; //*cookies per click!!!!!!!!!!!!

    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        base_cps += static_cast<double>(state.buildingsOwned[i]) *
                    buildingsDefinitions[i].base_cps;
        // * upgrades for each building
    };
    state.cps = base_cps; // * global_upgrades * buffs * ... TO BE IMPLEMENTED!!!!!!!!
    state.current_cookies += state.cps * dt;
    state.alltime_cookies += state.cps * dt;
}
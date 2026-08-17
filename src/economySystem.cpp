#include "economySystem.hpp"
#include "eventTypes.hpp"

#include <algorithm>
#include <stdexcept>

#include <cassert>
#include <cmath>

/*
HELPER FUNCTIONS
RETURN THE MULTIPLIERS FOR EACH UPGRADE FOR EACH SPECIFIC BUILDING
*/
namespace
{
    bool hasUpgrade(const GameState &state, const UpgradeType upgrade)
    {
        return state.upgradesOwned[static_cast<std::size_t>(+upgrade)];
    }

    double getCursorMultiplier(const GameState &state)
    {
        double multiplier = 1.0;

        if (hasUpgrade(state, UpgradeType::REINFORCED_INDEX_FINGER))
        {
            multiplier *= 2.0;
        }

        if (hasUpgrade(state, UpgradeType::CARPAL_TUNNEL_PREVENTION_CREAM))
        {
            multiplier *= 2.0;
        }

        if (hasUpgrade(state, UpgradeType::AMBIDEXTROUS))
        {
            multiplier *= 2.0;
        }

        return multiplier;
    }

    double getBuildingMultiplier(const GameState &state, const BuildingType building)
    {
        double multiplier = 1.0;

        switch (building)
        {
        case BuildingType::CURSOR:
            return getCursorMultiplier(state);

        case BuildingType::GRANDMA:
            if (hasUpgrade(state, UpgradeType::FORWARDS_FROM_GRANDMA))
            {
                multiplier *= 2.0;
            }

            if (hasUpgrade(state, UpgradeType::STEEL_PLATED_ROLLING_PINS))
            {
                multiplier *= 2.0;
            }

            if (hasUpgrade(state, UpgradeType::LUBRICATED_DENTURES))
            {
                multiplier *= 2.0;
            }

            return multiplier;

        case BuildingType::FARM:
            if (hasUpgrade(state, UpgradeType::CHEAP_HOES))
            {
                multiplier *= 2.0;
            }

            if (hasUpgrade(state, UpgradeType::FERTILIZER))
            {
                multiplier *= 2.0;
            }

            if (hasUpgrade(state, UpgradeType::COOKIE_TREES))
            {
                multiplier *= 2.0;
            }

            return multiplier;

        case BuildingType::MINE:
            if (hasUpgrade(state, UpgradeType::SUGAR_GAS))
            {
                multiplier *= 2.0;
            }

            return multiplier;

        case BuildingType::FACTORY:

        case BuildingType::BUILDING_COUNT:
            return multiplier;
        }

        throw std::logic_error("UNKNOWN BUILDING TYPE!!!!!!!!!!!!");
    }

    double getThousandFingersBonus(const GameState &state)
    {
        if (!hasUpgrade(state, UpgradeType::THOUSAND_FINGERS))
        {
            return 0.0;
        }

        int non_cursor_buildings = 0;

        for (int i = 0; i < +BuildingType::BUILDING_COUNT; ++i)
        {
            if (i != +BuildingType::CURSOR)
            {
                non_cursor_buildings += state.buildingsOwned[static_cast<std::size_t>(i)];
            }
        }

        return 0.1 * static_cast<double>(non_cursor_buildings);
    }
}

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
    const double thousand_fingers_bonus = getThousandFingersBonus(state);
    for (int i = 0; i < +BuildingType::BUILDING_COUNT; ++i)
    {
        const BuildingType building = static_cast<BuildingType>(i);

        double per_building_cps = buildingsDefinitions[static_cast<std::size_t>(i)].base_cps *
                                  getBuildingMultiplier(state, building);

        if (building == BuildingType::CURSOR)
        {
            // THOUSAND FINGERS ONLY adds to each CURSOR after the
            // first three Cursor doubling upgrades
            per_building_cps += thousand_fingers_bonus;
        }

        building_cps += static_cast<double>(state.buildingsOwned[static_cast<std::size_t>(i)]) * per_building_cps;
    };

    double clicking_cps = 0.0;
    if (is_clicking_active)
    {
        clicking_cps =
            Config::clicking_frequency *
            calculateCookiesPerClick(state);
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

    /*
    CURSOR cps per cursor: 0.1 x first-three-upgrade multiplier + THOUSAND-FINGERS bonus
    Cookies per click: 1.0 x first-three-upgrade multiplier + THOUSAND-FINGERS bonus
    */
    double effective_cps = (building_cps + clicking_cps * clicking_multiplier) * global_multiplier;

#ifndef NDEBUG
    assert(std::isfinite(effective_cps));
    assert(effective_cps >= 0.0);
#endif

    return effective_cps;
}

void EconomySystem::integrateOverDT(
    GameState &state,
    const double dt,
    const bool is_clicking_active)
{
#ifndef NDEBUG
    assert(std::isfinite(dt));
    assert(dt >= 0.0);

    const double previous_current_cookies =
        state.current_cookies;

    const double previous_alltime_cookies =
        state.alltime_cookies;
#endif

    constexpr double time_epsilon = 1e-9;

    // prevent negative dt
    if (dt < -time_epsilon)
    {
        throw std::logic_error("NEGATIVE DT ???????????????????");
    }

    double positive_dt = std::max(0.0, dt);

    double rate = this->calculateEffectiveCPS(state, is_clicking_active);

#ifndef NDEBUG
    assert(std::isfinite(rate));
    assert(rate >= 0.0);
#endif

    state.current_cookies += rate * positive_dt;
    state.alltime_cookies += rate * positive_dt;
    state.total_cps = rate;

#ifndef NDEBUG
    assert(std::isfinite(state.current_cookies));
    assert(std::isfinite(state.alltime_cookies));
    assert(std::isfinite(state.total_cps));

    assert(
        state.current_cookies >=
        previous_current_cookies);

    assert(
        state.alltime_cookies >=
        previous_alltime_cookies);

    assert(state.total_cps >= 0.0);
#endif
}

double EconomySystem::calculateCookiesPerClick(const GameState &state)
{
    // first three CURSOR upgrades double the base mouse cps
    // THOUSAND FINGERS is then added separately
    const double cookies_per_click = state.cookies_per_click * getCursorMultiplier(state) + getThousandFingersBonus(state);

#ifndef NDEBUG
    assert(std::isfinite(cookies_per_click));
    assert(cookies_per_click >= 0.0);
#endif

    return cookies_per_click;
}

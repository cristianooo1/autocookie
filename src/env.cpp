#include "env.hpp"

Env::Env()
{
}

StepResult Env::step(const Action &action)
{

    /*

    if (action.type == ActionType::BuyBuilding)
    {
        buildingSystem.buy(state, action);

        constexpr double dt = Config::buying_time_cost;

        simulationSystem.advanceTime(state, dt);
            where: state.current_time += dt;
        economy.advance(state, dt, false);
        eventSystem.processEvents(state);
    }
    else if (action.type == ActionType::Advance)
    {
        double dt = simulationSystem.timeUntilNextDecision(state);
            //where it determines time passed until next relevant event
            // ignore events that are currently available: e.g. cursor is availabe to buy but agent doesnt want to
            // cap dt by remaining episode time

        simulationSystem.advanceTime(state, dt);
        economy.advance(state, dt, true);
        eventSystem.processEvents(state);
    }

    return {
        get_observation(),
        get_reward(),
        is_terminal()
    };

    */

    this->buildingSystem.update(state, action);
    state.current_time += dt;
    // events.update(state, dt);
    economy.update(state, dt);

    return StepResult{
        .obs = get_observation(),
        .reward = get_reward(),
        .done = is_terminal(),
    };
}

Observation Env::reset()
{
    state.current_time = 0.0;
    state.current_cookies = 0.0;
    state.alltime_cookies = 0.0;
    state.cps = 0.0;
    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        state.buildingsOwned[i] = 0;
    }

    prev_progress_cookies = 0.0;
    prev_progress_alltime_cookies = 0.0;
    prev_progress_cps = 0.0;

    Observation obs;
    obs.current_cookies = state.current_cookies,
    obs.all_time_cookies = state.alltime_cookies,
    obs.cps = state.cps,
    obs.buildings_owned = state.buildingsOwned;

    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        obs.can_buy_1[i] = false;
        obs.can_buy_10[i] = false;
        obs.can_buy_100[i] = false;
    }

    return obs;
}

Observation Env::get_observation()
{
    Observation obs;
    obs.current_cookies = state.current_cookies,
    obs.all_time_cookies = state.alltime_cookies,
    obs.cps = state.cps,
    obs.buildings_owned = state.buildingsOwned;

    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        obs.can_buy_1[i] = buildingSystem.can_buy(state, i, 1);
        obs.can_buy_10[i] = buildingSystem.can_buy(state, i, 10);
        obs.can_buy_100[i] = buildingSystem.can_buy(state, i, 100);
    }

    return obs;
}

double Env::get_reward()
{
    // CHANGE THIS!!!!!!!!!!!
    double reward = (state.alltime_cookies - prev_progress_alltime_cookies) + (state.cps - prev_progress_cps) * 5;
    prev_progress_alltime_cookies = state.alltime_cookies;
    prev_progress_cps = state.cps;
    return reward;
}

bool Env::is_terminal()
{
    if (state.current_time >= Config::episode_length)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::tuple<double, double, double, double, int, int, int> Env::queryState()
{
    // std::cout << "Current Cookies: " << this->state.current_cookies << "\n";
    // std::cout << "All Time Cookies: " << this->state.alltime_cookies << "\n";
    // std::cout << "CPS: " << this->state.cps << "\n";
    // std::cout << "Cursors: " << this->state.buildingsOwned[+BuildingType::CURSOR] << "\n";
    // std::cout << "Grandmas: " << this->state.buildingsOwned[+BuildingType::GRANDMA] << "\n";
    // std::cout << "Farms: " << this->state.buildingsOwned[+BuildingType::FARM] << "\n";
    return std::make_tuple(
        state.current_cookies,
        state.alltime_cookies,
        state.cps,
        state.current_time,
        state.buildingsOwned[+BuildingType::CURSOR],
        state.buildingsOwned[+BuildingType::GRANDMA],
        state.buildingsOwned[+BuildingType::FARM]);
}

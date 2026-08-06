#include "env.hpp"

/*
SIMULATION STEPS:

Current state

Receive action = PLAYER COMMAND

Execute action BY ENGINE

Events occur

Recalculate production

Advance timers - simulation by dt

Return new state
*/

/*
RL TRAINING:
reset()

step(action)

getObservation()

getReward()

isTerminal()
*/

/*
https://www.learncpp.com/cpp-tutorial/scoped-enumerations-enum-classes/

Overload the unary + operator to convert an enum to the underlying type
*/
template <typename T>
constexpr auto operator+(T a) noexcept
{
    return static_cast<std::underlying_type_t<T>>(a);
}

void BuildingSystem::update(GameState &state, const Action &action)
{
    double price = Config::buildingsDefinitions[+action.buildingIndex].base_cost *
                   std::pow(
                       Config::buildingsDefinitions[+action.buildingIndex].cost_multiplier,
                       static_cast<double>(state.buildingsOwned[+action.buildingIndex]));

    if (action.type == ActionType::BuyBuilding && state.current_cookies >= price)
    {
        state.current_cookies -= price;
        state.buildingsOwned[+action.buildingIndex] += 1;
    }
    else if (action.type == ActionType::ClickCookie)
    {
        state.current_cookies += 1;
        state.alltime_cookies += 1;
    }
    else if (action.type == ActionType::Wait)
    {
    }
}

void Economy::update(GameState &state, const double dt)
{
    double base_cps = 0.0;
    for (int i = 0; i < Config::TOTAL_NR_BUILDINGS_AVAILABLE; i++)
    {
        base_cps += static_cast<double>(state.buildingsOwned[i]) * Config::buildingsDefinitions[i].base_production;
    };
    state.cps = base_cps; // * upgrades * buffs * ... TO BE IMPLEMENTED!!!!!!!!
    state.current_cookies += state.cps * dt;
    state.alltime_cookies += state.cps * dt;
}

StepResult Env::step(const Action &action)
{
    buildingSystem.update(state, action);
    state.time += dt;
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
    state.alltime_cookies = 0.0;
    state.current_cookies = 0.0;
    state.time = 0.0;
    state.cps = 0.0;
    for (int i = 0; i < Config::TOTAL_NR_BUILDINGS_AVAILABLE; i++)
    {
        state.buildingsOwned[i] = 0;
    }

    prev_progress_cookies = 0.0;
    prev_progress_alltime_cookies = 0.0;
    prev_progress_cps = 0.0;

    return Observation{
        .current_cookies = state.current_cookies,
        .all_time_cookies = state.alltime_cookies,
        .buildings_owned = state.buildingsOwned,
        .cps = state.cps,
    };
}

Observation Env::get_observation()
{
    return Observation{
        .current_cookies = state.current_cookies,
        .all_time_cookies = state.alltime_cookies,
        .buildings_owned = state.buildingsOwned,
        .cps = state.cps,
    };
}

double Env::get_reward()
{
    double reward = (state.alltime_cookies - prev_progress_alltime_cookies) + (state.cps - prev_progress_cps) * 5;
    prev_progress_alltime_cookies = state.alltime_cookies;
    prev_progress_cps = state.cps;
    return reward;
}

bool Env::is_terminal()
{
    if (state.time == 100)
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
        state.time,
        state.buildingsOwned[+BuildingType::CURSOR],
        state.buildingsOwned[+BuildingType::GRANDMA],
        state.buildingsOwned[+BuildingType::FARM]);
}

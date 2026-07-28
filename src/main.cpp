#include <iostream>
#include <string>
#include <chrono>
#include <tuple>
#include <vector>
#include <memory> //for std::unique_ptr
#include <cmath>
/*
Current state

Receive action

Execute action

Advance simulation by dt

Events occur

Recalculate production

Advance timers

Return new state
*/

namespace Cursor
{
    constexpr double base_production{0.1}; // cps
    constexpr double base_cost{10.0};      // cookies
    constexpr double cost_multiplier{1.15};
}

namespace Grandma
{
    constexpr double base_production{0.3}; // cps
    constexpr double base_cost{30.0};      // cookies
    constexpr double cost_multiplier{1.25};
}

enum class Action
{
    buyCursor,
    buyGrandma,
    Wait,
};

struct BuildingSystem
{
    void update(GameState &state, const Action &action)
    {

        if (action == Action::buyCursor &&
            state.cookies >= (Cursor::base_cost * std::pow(Cursor::cost_multiplier, static_cast<double>(state.cursorCount))))

        {
            state.cursorCount += 1;
            state.cookies -= (Cursor::base_cost * std::pow(Cursor::cost_multiplier, static_cast<double>(state.cursorCount)));
        }
        else if (action == Action::buyGrandma &&
                 state.cookies >= (Grandma::base_cost * std::pow(Grandma::cost_multiplier, static_cast<double>(state.grandmaCount))))
        {
            state.grandmaCount += 1;
            state.grandmaCount -= (Grandma::base_cost * std::pow(Grandma::cost_multiplier, static_cast<double>(state.grandmaCount)));
        }
        else if (action == Action::Wait)
        {
        }
    }
};

struct Economy
{
    void update(GameState &state, const double dt)
    {
        state.cps += 0.1 * state.cursorCount + 0.3 * state.grandmaCount;
        state.cookies += state.cps * dt;
    }
};

struct Engine
{
    const double dt{1.0};
    GameState state;
    Economy economy;
    BuildingSystem buildingSystem;

    void step(const Action &action)
    {
        buildingSystem.update(state, action);
        state.time += dt;
        // events.update(state, dt);
        economy.update(state, dt);
    }

    void queryState()
    {
        std::cout << "Cookies: " << this->state.cookies << "\n";
        std::cout << "CPS: " << this->state.cps << "\n";
        std::cout << "Cursors: " << this->state.cursorCount << "\n";
    }
};

struct GameState
{
    double cookies{0.0};
    double cps{0.0};
    int cursorCount{0};
    int grandmaCount{0};
    double time{0.0};
    // active_buffs
    // upgrades_owned
};

main(int argc, char *argv[])
{
    Engine engine;

    Action action{Action::buyCursor};

    while (true)
    {
        engine.step(action);
        engine.queryState();
    }

    return 0;
}

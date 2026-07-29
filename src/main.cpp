#include <iostream>
#include <string>
#include <chrono>
#include <tuple>
#include <vector>
#include <memory> //for std::unique_ptr
#include <cmath>
#include <string>
#include <array>

/*
SIMULATION STEPS:

Current state

Receive action

Execute action

Advance simulation by dt

Events occur

Recalculate production

Advance timers

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

enum class BuildingType
{
    CURSOR = 0,
    GRANDMA = 1,
    FARM = 2,
};

enum class ActionType
{
    BuyBuilding,
    Wait,
    // ClickGoldenCookie,
    // Ascend,

};

// https://www.learncpp.com/cpp-tutorial/scoped-enumerations-enum-classes/
template <typename T>
constexpr auto operator+(T a) noexcept
{
    return static_cast<std::underlying_type_t<T>>(a);
}

struct BuildingDefinition
{
    double base_cost{0.1};
    double base_production{0.1};
    double cost_multiplier{0.1};
};

struct Action
{
    ActionType type{};
    BuildingType buildingIndex{};
};

namespace Config
{
    constexpr int TOTAL_NR_BUILDINGS_AVAILABLE = 3;
    std::array<BuildingDefinition, TOTAL_NR_BUILDINGS_AVAILABLE> buildingsDefinitions{
        BuildingDefinition{
            // CURSOR = 0
            .base_cost = 1,
            .base_production = 0.1,
            .cost_multiplier = 1.15,
        },
        BuildingDefinition{
            // GRANDMA = 1
            .base_cost = 20,
            .base_production = 0.25,
            .cost_multiplier = 1.25,
        },
        BuildingDefinition{
            // FARM = 2
            .base_cost = 50,
            .base_production = 0.4,
            .cost_multiplier = 1.5,
        }};
};

struct GameState
{

    /*
    0 -> cursor
    1 -> grandma
    2 -> farm
    */
    std::array<int, Config::TOTAL_NR_BUILDINGS_AVAILABLE> buildingsOwned{
        0, // number of CURSORS owned
        0, // number of GRANDMAS owned
        0, // number of FARMS owned
    };

    double cookies{10.0};
    double cps{0.0};

    double time{0.0};
    // active_buffs
    // upgrades_owned
};

struct BuildingSystem
{

    void update(GameState &state, const Action &action)
    {
        double price = Config::buildingsDefinitions[+action.buildingIndex].base_cost *
                       std::pow(
                           Config::buildingsDefinitions[+action.buildingIndex].cost_multiplier,
                           static_cast<double>(state.buildingsOwned[+action.buildingIndex]));

        if (action.type == ActionType::BuyBuilding && state.cookies >= price)
        {
            state.cookies -= price;
            state.buildingsOwned[+action.buildingIndex] += 1;
        }
        else if (action.type == ActionType::Wait)
        {
        }
    }
};

struct Economy
{
    void update(GameState &state, const double dt)
    {
        double base_cps = 0.0;
        for (int i = 0; i < Config::TOTAL_NR_BUILDINGS_AVAILABLE; i++)
        {
            base_cps += static_cast<double>(state.buildingsOwned[i]) * Config::buildingsDefinitions[i].base_production;
        };
        state.cps = base_cps; // * upgrades * buffs * ... TO BE IMPLEMENTED!!!!!!!!
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
        std::cout << "Cursors: " << this->state.buildingsOwned[+BuildingType::CURSOR] << "\n";
        std::cout << "Grandmas: " << this->state.buildingsOwned[+BuildingType::GRANDMA] << "\n";
        std::cout << "Farms: " << this->state.buildingsOwned[+BuildingType::FARM] << "\n";
    }
};

int main(int argc, char *argv[])
{
    Engine engine;

    Action action1{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::CURSOR};
    Action action2{.type = ActionType::Wait};
    Action action3{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::FARM};

    engine.step(action1);
    engine.step(action2);
    engine.step(action2);
    engine.queryState();

    return 0;
}

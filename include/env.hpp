#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <tuple>
#include <vector>
#include <memory> //for std::unique_ptr
#include <cmath>
#include <string>

#include "config.hpp"

enum class BuildingType
{
    CURSOR = 0,
    GRANDMA = 1,
    FARM = 2,
};

enum class ActionType
{
    ClickCookie = 0,
    BuyBuilding,
    Wait,
    // ClickGoldenCookie,
    // Ascend,

};

struct Action
{
    ActionType type{};
    BuildingType buildingIndex{};
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

    double current_cookies{0.0};
    double alltime_cookies{0.0};
    double cps{0.0};

    double time{0.0};
    // active_buffs
    // upgrades_owned
};

struct BuildingSystem
{

    void update(GameState &state, const Action &action);
};

struct Economy
{
    void update(GameState &state, const double dt);
};

struct Observation
{
    double current_cookies{};
    double all_time_cookies{};
    std::array<int, Config::TOTAL_NR_BUILDINGS_AVAILABLE> buildings_owned{};
    double cps{};
};

struct StepResult
{
    Observation obs;
    double reward;
    bool done;
};

struct Env
{
    const double dt{1.0};
    GameState state;
    Economy economy;
    BuildingSystem buildingSystem;

    double prev_progress_cookies{0.0};
    double prev_progress_alltime_cookies{0.0};
    double prev_progress_cps{0.0};

    StepResult step(const Action &action);

    Observation reset();

    Observation get_observation();

    double get_reward();

    bool is_terminal();

    std::tuple<double, double, double, double, int, int, int> queryState();
};

#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <tuple>
#include <vector>
#include <memory> //for std::unique_ptr
#include <string>

#include "config.hpp"
#include "types.hpp"
#include "gameState.hpp"
#include "buildingSystem.hpp"
#include "economySystem.hpp"

struct Observation
{
    double current_cookies{};
    double all_time_cookies{};
    double cps{};
    std::array<int, +BuildingType::BUILDING_COUNT> buildings_owned{};

    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_1{};
    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_10{};
    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_100{};

    // active buffs and timers
};

struct StepResult
{
    Observation obs;
    double reward;
    bool done;
};

class Env
{
public:
    Env();

    StepResult step(const Action &action);

    Observation reset();

    Observation get_observation();

    double get_reward();

    bool is_terminal();

    std::tuple<double, double, double, double, int, int, int> queryState();

private:
    GameState state;
    Economy economy;
    BuildingSystem buildingSystem;

    const double dt{1.0};

    double prev_progress_cookies{0.0};
    double prev_progress_alltime_cookies{0.0};
    double prev_progress_cps{0.0};
};

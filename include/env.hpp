#pragma once

#include <iostream>
#include <chrono>
#include <tuple>
#include <vector>
#include <memory> //for std::unique_ptr
#include <string>
#include <array>
#include <optional>

#include "config.hpp"
#include "types.hpp"
#include "gameState.hpp"
#include "buildingSystem.hpp"
#include "economySystem.hpp"
#include "simulationSystem.hpp"
#include "eventSystem.hpp"

#include "eventTypes.hpp"

struct Observation
{
    double current_simulation_time{};
    double current_cookies{};
    double all_time_cookies{};
    double total_cps{};
    std::array<int, +BuildingType::BUILDING_COUNT> buildings_owned{};

    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_1{};
    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_10{};
    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_100{};

    std::vector<ActiveGoldenCookieBuff> activeGoldenCookieBuffs{};
};

struct StepResult
{
    Observation obs;
    double reward;
    bool done;
};

struct EnvTestAccess;

class Env
{
public:
    Env();

    StepResult step(const Action &action);

    Observation reset(std::optional<unsigned int> seed = std::nullopt);

    Observation get_observation();

    double get_reward();

    bool is_terminal();

    std::tuple<double, double, double, double, int, int, int> queryState();

    unsigned int getEpisodeSeed();

private:
    GameState state;
    EconomySystem economySystem;
    BuildingSystem buildingSystem;
    EventSystem eventSystem;
    SimulationSystem simulationSystem;

    friend struct EnvTestAccess;

    double prev_progress_alltime_cookies{0.0};
    double prev_progress_cps{0.0};
};

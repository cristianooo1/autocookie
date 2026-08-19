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
#include "upgradeSystem.hpp"

#include "eventTypes.hpp"

struct Observation
{
    double current_simulation_time{0.0};
    double current_cookies{0.0};
    double all_time_cookies{0.0};
    double handmade_cookies{0.0};
    double total_cps{0.0};

    double seconds_since_last_golden_cookie{0.0};
    bool has_seen_golden_cookie{false};

    std::array<int, +BuildingType::BUILDING_COUNT> buildings_owned{};

    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_1{};
    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_10{};
    std::array<bool, +BuildingType::BUILDING_COUNT> can_buy_100{};

    // these contain the active buffs and their remaining timers
    // absolute expiration timestamps, future spawn timestamps, future outcomes remain private
    std::vector<GoldenCookieBuff> activeGoldenCookieBuffs{};
    std::vector<double> activeGoldenCookieBuffSecondsRemaining{};

    std::array<bool, +UpgradeType::UPGRADE_COUNT> upgrades_owned{};
    std::array<bool, +UpgradeType::UPGRADE_COUNT> upgrades_unlocked{};
    std::array<bool, +UpgradeType::UPGRADE_COUNT> can_buy_upgrades{};

    /*
    FIXED-SIZE POLICY MASK
        ADVANCE are always valid
        PURCHASE actions are valid only when they are unlocked
    */
    std::array<bool, discreteActionCount> valid_action_mask{};
};

enum class EpisodeOutcome
{
    Ongoing,
    Success,
    HorizonFailure,
};

struct StepResult
{
    Observation obs{};
    double reward{0.0};

    EpisodeOutcome outcome{EpisodeOutcome::Ongoing};

    bool reached_target{false};
    bool reached_horizon{false};

    bool terminated{false};
    bool truncated{false};

    // [[deprecated("Use terminated OR truncated params !!!!!!!!!!!!")]]
    // bool done{false};
};

struct EnvTestAccess;

class Env
{
public:
    Env();

    StepResult step(const Action &action);

    Observation reset(std::optional<unsigned int> seed = std::nullopt);

    Observation get_observation();

    // double get_reward();

    bool is_terminal();

    std::tuple<double, double, double, double, int, int, int> queryState();

    unsigned int getEpisodeSeed();

private:
    GameState state;
    EconomySystem economySystem;
    BuildingSystem buildingSystem;
    EventSystem eventSystem;
    SimulationSystem simulationSystem;
    UpgradeSystem upgradeSystem;

    friend struct EnvTestAccess;

    // double prev_reward_time{0.0};
    // double prev_progress_alltime_cookies{0.0};
    // double prev_progress_cps{0.0};
};

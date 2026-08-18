#include "env.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cassert>

namespace
{
    struct EpisodeStatus
    {
        EpisodeOutcome outcome{EpisodeOutcome::Ongoing};
        bool reached_target{false};
        bool reached_horizon{false};
    };

    EpisodeStatus classifyEpisode(
        const GameState &state)
    {
        const bool reached_target =
            state.alltime_cookies >=
            Config::target_cookies;

        const bool reached_horizon =
            state.current_simulation_time >=
            Config::episode_length;

        EpisodeOutcome outcome =
            EpisodeOutcome::Ongoing;

        // Success takes precedence when the target and horizon coincide
        if (reached_target)
        {
            outcome = EpisodeOutcome::Success;
        }
        else if (reached_horizon)
        {
            outcome = EpisodeOutcome::HorizonFailure;
        }

        return EpisodeStatus{
            .outcome = outcome,
            .reached_target = reached_target,
            .reached_horizon = reached_horizon,
        };
    }

    struct RewardSnapshot
    {
        double simulation_time{0.0};
        double alltime_cookies{0.0};
        double total_cps{0.0};
    };

    RewardSnapshot captureRewardSnapshot(
        const GameState &state)
    {
        return RewardSnapshot{
            .simulation_time =
                state.current_simulation_time,
            .alltime_cookies =
                state.alltime_cookies,
            .total_cps =
                state.total_cps,
        };
    }

    double progressPotential(
        const double alltime_cookies)
    {
        const double capped_cookies =
            std::min(
                alltime_cookies,
                Config::target_cookies);

        return std::log1p(capped_cookies) /
               std::log1p(Config::target_cookies);
    }

    double calculateTransitionReward(
        const RewardSnapshot &before,
        const GameState &after,
        const EpisodeOutcome outcome)
    {
        const double delta_time =
            after.current_simulation_time -
            before.simulation_time;

        const bool success =
            outcome == EpisodeOutcome::Success;

        const double base_reward =
            -delta_time / Config::episode_length +
            (success ? 1.0 : 0.0);

        double reward = 0.0;

        switch (Config::reward_mode)
        {
        case Config::RewardMode::TimeSuccess:
            reward = base_reward;
            break;

        case Config::RewardMode::TimeSuccessLogPotential:
            reward =
                base_reward +
                Config::progress_shaping_beta *
                    (progressPotential(after.alltime_cookies) -
                     progressPotential(before.alltime_cookies));
            break;

        case Config::RewardMode::OriginalCookiesPlusCps:
            reward =
                (after.alltime_cookies -
                 before.alltime_cookies) +
                (after.total_cps -
                 before.total_cps) *
                    5.0;
            break;
        }

#ifndef NDEBUG
        assert(std::isfinite(delta_time));
        assert(delta_time >= 0.0);
        assert(std::isfinite(reward));
#endif

        return reward;
    }
}

#ifndef NDEBUG
namespace
{
    void assertValidDecisionState(const GameState &state)
    {
        assert(std::isfinite(state.current_simulation_time));
        assert(std::isfinite(state.current_cookies));
        assert(std::isfinite(state.alltime_cookies));
        assert(std::isfinite(state.handmade_cookies));
        assert(std::isfinite(
            state.last_golden_cookie_timestamp));
        assert(std::isfinite(state.total_cps));
        assert(std::isfinite(state.cookies_per_click));

        assert(state.current_simulation_time >= 0.0);
        assert(state.current_simulation_time <= Config::episode_length);
        assert(state.current_cookies >= 0.0);
        assert(state.alltime_cookies >= 0.0);
        assert(state.handmade_cookies >= 0.0);
        assert(state.handmade_cookies <= state.alltime_cookies);
        assert(state.last_golden_cookie_timestamp >= 0.0);

        if (state.has_seen_golden_cookie)
        {
            assert(
                state.last_golden_cookie_timestamp <=
                state.current_simulation_time);
        }
        assert(state.current_cookies <= state.alltime_cookies);
        assert(state.total_cps >= 0.0);
        assert(state.cookies_per_click >= 0.0);

        for (int count : state.buildingsOwned)
        {
            assert(count >= 0);
        }

        assert(std::is_sorted(
            state.activeGoldenCookieBuffs.begin(),
            state.activeGoldenCookieBuffs.end(),
            [](const ActiveGoldenCookieBuff &a,
               const ActiveGoldenCookieBuff &b)
            {
                return a.expires_at < b.expires_at;
            }));

        for (std::size_t i = 0;
             i < state.activeGoldenCookieBuffs.size();
             ++i)
        {
            const ActiveGoldenCookieBuff &buff =
                state.activeGoldenCookieBuffs[i];

            const int buff_index =
                static_cast<int>(buff.buff_type);

            assert(buff_index >= 0);
            assert(
                buff_index <
                +GoldenCookieBuff::GOLDEN_COOKIE_BUFF_COUNT);

            const GoldenCookieBuffDefinition &definition =
                GoldenCookieBuff_Definitions[static_cast<std::size_t>(buff_index)];

            // LUCKY is instantaneous
            // disabled buffs cannot be active
            assert(definition.weight > 0.0);
            assert(definition.duration > 0.0);

            assert(std::isfinite(buff.starts_at));
            assert(std::isfinite(buff.expires_at));
            assert(buff.starts_at >= 0.0);
            assert(buff.starts_at <= state.current_simulation_time);
            assert(buff.expires_at > buff.starts_at);

            assert(buff.expires_at >= state.current_simulation_time);

            if (state.current_simulation_time < Config::episode_length &&
                state.alltime_cookies < Config::target_cookies)
            {
                assert(
                    buff.expires_at >
                    state.current_simulation_time);
            }

            for (std::size_t j = i + 1;
                 j < state.activeGoldenCookieBuffs.size();
                 ++j)
            {
                assert(
                    buff.buff_type !=
                    state.activeGoldenCookieBuffs[j].buff_type);
            }
        }
    }
}
#endif

Env::Env()
{
}

StepResult Env::step(const Action &action)
{
#ifndef NDEBUG
    const double step_start_time =
        state.current_simulation_time;

    assertValidDecisionState(state);
#endif

    const EpisodeStatus initial_status =
        classifyEpisode(state);

    if (initial_status.outcome !=
        EpisodeOutcome::Ongoing)
    {
        return StepResult{
            .obs = get_observation(),
            .reward = 0.0,
            .outcome = initial_status.outcome,
            .reached_target =
                initial_status.reached_target,
            .reached_horizon =
                initial_status.reached_horizon,
            .terminated = true,
            .truncated = false,
            .done = true,
        };
    }

    const RewardSnapshot reward_before =
        captureRewardSnapshot(state);

    /*
    HELPER FUNCTION
    returns true if if finds a specific event in a vector of events
    */
    auto is_event_here =
        [](const std::vector<NextScheduledEvent> &events,
           const WakeUpDecisionEventType type)
    {
        return std::any_of(
            events.begin(),
            events.end(),
            [type](const NextScheduledEvent &event)
            {
                return event.event_type == type;
            });
    };

    /*
    HELPER FUNCTION
    removes tiny floating-point error
    */
    auto snap_to_target_boundary =
        [&]()
    {
        const double correction =
            Config::target_cookies - state.alltime_cookies;

        state.current_cookies += correction;
        state.alltime_cookies = Config::target_cookies;

#ifndef NDEBUG
        assert(std::isfinite(state.current_cookies));
        assert(state.current_cookies >= 0.0);
        assert(
            state.alltime_cookies ==
            Config::target_cookies);
#endif
    };

    /*
    HELPER FUNCTIONS
    processes all events at current gamestate in a specific order:
    1. remove expired buffs
    2. add new buffs
    3. process everything
    */
    auto remove_expired_buffs_at_current_time =
        [&](const std::vector<NextScheduledEvent> &events)
    {
        if (is_event_here(
                events,
                WakeUpDecisionEventType::
                    GoldenCookieBuffExpiration))
        {
#ifndef NDEBUG
            assert(std::any_of(
                state.activeGoldenCookieBuffs.begin(),
                state.activeGoldenCookieBuffs.end(),
                [&](const ActiveGoldenCookieBuff &buff)
                {
                    return buff.expires_at ==
                           state.current_simulation_time;
                }));
#endif
            eventSystem.removeGoldenCookieBuff(state);
        } };

    // LUCKY NEEDS RATE AFTER ALL BUFFS EXPIRE!!!!!!!!!!!
    auto process_golden_cookie_spawn_at_current_time =
        [&](const std::vector<NextScheduledEvent> &events)
    {
        if (is_event_here(
                events,
                WakeUpDecisionEventType::
                    GoldenCookieSpawned))
        {
#ifndef NDEBUG
            assert(
                state.current_simulation_time ==
                eventSystem.getNextGoldenCookieSpawn());
#endif
            // LUCKY uses passive cps
            double rate =
                economySystem.calculateEffectiveCPS(
                    state,
                    false);
#ifndef NDEBUG
            assert(std::isfinite(rate));
            assert(rate >= 0.0);
#endif

            eventSystem.processGoldenCookieBuff(
                state,
                rate);
        }
    };

    auto process_events_at_current_time =
        [&](const std::vector<NextScheduledEvent> &events)
    {
        remove_expired_buffs_at_current_time(events);
        process_golden_cookie_spawn_at_current_time(events);
    };

    bool episode_ended = false;

    if (action.type == ActionType::Advance)
    {
        const bool clicking = true;

        const double passive_rate =
            economySystem.calculateEffectiveCPS(state, false);

        const double rate =
            economySystem.calculateEffectiveCPS(state, true);

        const double clicking_rate =
            std::max(0.0, rate - passive_rate);

        // get closest future wakeup events = building affordable OR golden cookie spawn OR buff expiration
        std::vector<NextScheduledEvent> future_events = simulationSystem.getTimeNextDecision(
            state,
            buildingSystem,
            upgradeSystem,
            eventSystem,
            rate,
            clicking_rate);
        double next_timestamp = future_events.at(0).absolute_timestamp;

#ifndef NDEBUG
        assert(!future_events.empty());
        assert(std::isfinite(next_timestamp));
        assert(
            next_timestamp >
            state.current_simulation_time);
#endif

        // update economy with clicking=true
        double dt = next_timestamp - state.current_simulation_time;
        economySystem.integrateOverDT(state, dt, clicking);

        // advance clock to next event timestamp = t2
        state.current_simulation_time = next_timestamp;

        // process all events and check for episode finish

        if (is_event_here(
                future_events,
                WakeUpDecisionEventType::UpgradeUnlocked))
        {
            state.handmade_cookies =
                std::max(
                    state.handmade_cookies,
                    Config::plastic_mouse_unlock_cookies);
        }
        if (is_event_here(
                future_events,
                WakeUpDecisionEventType::TargetBoundary))
        {
            snap_to_target_boundary();
        }

        if (is_event_here(
                future_events,
                WakeUpDecisionEventType::EpisodeBoundary))
        {
            state.current_simulation_time =
                Config::episode_length;

            episode_ended = true;
        }

        if (!episode_ended &&
            state.alltime_cookies <
                Config::target_cookies)
        {
            process_events_at_current_time(future_events);
        }
    }
    else if (action.type == ActionType::BuyBuilding ||
             action.type == ActionType::BuyUpgrade)
    {
        // START = T1

        PurchaseIntent building_purchase{};
        UpgradePurchaseIntent upgrade_purchase{};

        bool quoted_can_complete = false;

        if (action.type == ActionType::BuyBuilding)
        {
            building_purchase = buildingSystem.validatePurchase(state, action);

            quoted_can_complete = building_purchase.canAfford;
        }
        else
        {
            upgrade_purchase = upgradeSystem.validatePurchase(state, action);

            quoted_can_complete = upgrade_purchase.canPurchase;
        }

        auto complete_quoted_purchase =
            [&]()
        {
            if (action.type ==
                ActionType::BuyBuilding)
            {
                buildingSystem.makePurchase(
                    state,
                    building_purchase);
            }
            else
            {
                upgradeSystem.makePurchase(
                    state,
                    upgrade_purchase);
            }
        };

        bool clicking = false;
        bool has_purchase_completed = false;
        double purchase_end_timestamp = state.current_simulation_time + Config::buying_time_cost;

#ifndef NDEBUG
        assert(
            std::isfinite(
                purchase_end_timestamp));

        assert(
            purchase_end_timestamp >
            state.current_simulation_time);
#endif

        // get closest future wakeup event/s = building affordable AND/OR golden cookie spawn AND/OR buff expiration
        while (state.current_simulation_time < purchase_end_timestamp)
        {
            // t1 -> event.timestamp -> t2 = buy_building

            double rate =
                economySystem.calculateEffectiveCPS(state, false);

            std::vector<NextScheduledEvent> future_events = simulationSystem.getTimeNextInternalEvents(
                state,
                eventSystem,
                rate);

            double internal_timestamp = future_events.at(0).absolute_timestamp;

            double next_timestamp = std::min(purchase_end_timestamp, internal_timestamp);

#ifndef NDEBUG
            assert(std::isfinite(next_timestamp));
            assert(
                next_timestamp >
                state.current_simulation_time);
            assert(
                next_timestamp <=
                purchase_end_timestamp);
#endif

            // update economy with clicking=false t1 -> event.timestamp
            double dt = next_timestamp - state.current_simulation_time;
            economySystem.integrateOverDT(state, dt, clicking);

            // advance clock to event.timestamp
            state.current_simulation_time = next_timestamp;

            // process all events at event.timestamp
            bool is_internal_timestamp_reached = internal_timestamp == next_timestamp;

            if (is_internal_timestamp_reached &&
                is_event_here(
                    future_events,
                    WakeUpDecisionEventType::TargetBoundary))
            {
                snap_to_target_boundary();
            }

            if (is_internal_timestamp_reached &&
                is_event_here(
                    future_events,
                    WakeUpDecisionEventType::EpisodeBoundary))
            {
                state.current_simulation_time =
                    Config::episode_length;

                episode_ended = true;
            }

            if (state.alltime_cookies >=
                    Config::target_cookies ||
                episode_ended)
            {
                break;
            }

            if (is_internal_timestamp_reached)
            {
                // expiration -> purchase completion -> spawn.
                remove_expired_buffs_at_current_time(future_events);

                if (next_timestamp == purchase_end_timestamp && quoted_can_complete)
                {
                    complete_quoted_purchase();
                    has_purchase_completed = true;
                }

                process_golden_cookie_spawn_at_current_time(
                    future_events);

                if (state.alltime_cookies >= Config::target_cookies)
                {
                    break;
                }
            }

            if (state.current_simulation_time >= purchase_end_timestamp)
            {
                break;
            }
        }

        if (!episode_ended &&
            state.alltime_cookies < Config::target_cookies &&
            quoted_can_complete &&
            !has_purchase_completed)
        {
            complete_quoted_purchase();
        }

#ifndef NDEBUG
        if (state.alltime_cookies >=
            Config::target_cookies)
        {
            assert(
                state.current_simulation_time <=
                purchase_end_timestamp);
        }
        else if (!episode_ended)
        {
            assert(
                state.current_simulation_time ==
                purchase_end_timestamp);
        }
        else
        {
            assert(
                state.current_simulation_time ==
                Config::episode_length);
        }
#endif
    }
    else
    {
        throw std::invalid_argument("UNKNOW ACTION!!!!!!!!!!!!!!!!!!!!");
    }

    state.total_cps = economySystem.calculateEffectiveCPS(state, true);

#ifndef NDEBUG
    assert(
        state.current_simulation_time >=
        step_start_time);

    assertValidDecisionState(state);

    assert(
        state.total_cps ==
        economySystem.calculateEffectiveCPS(
            state,
            true));
#endif

    const EpisodeStatus status =
        classifyEpisode(state);

    const double reward =
        calculateTransitionReward(
            reward_before,
            state,
            status.outcome);

    const bool terminated =
        status.outcome !=
        EpisodeOutcome::Ongoing;

    constexpr bool truncated{false};

    return StepResult{
        .obs = get_observation(),
        .reward = reward,
        .outcome = status.outcome,
        .reached_target = status.reached_target,
        .reached_horizon = status.reached_horizon,
        .terminated = terminated,
        .truncated = truncated,
        .done = terminated || truncated,
    };
}

Observation Env::reset(std::optional<unsigned int> seed)
{
    state = GameState{};

    if (seed.has_value())
    {
        eventSystem.setEpisodeSeed(*seed);
    }
    else
    {
        eventSystem.generateEpisodeSeed();
    }

    eventSystem.generateNextGoldenCookieSpawn(state);

    state.total_cps = economySystem.calculateEffectiveCPS(state, true);

#ifndef NDEBUG
    assertValidDecisionState(state);

    assert(
        state.total_cps ==
        economySystem.calculateEffectiveCPS(
            state,
            true));

    assert(std::isfinite(
        eventSystem.getNextGoldenCookieSpawn()));

    assert(
        eventSystem.getNextGoldenCookieSpawn() >
        state.current_simulation_time);
#endif

    return get_observation();
}

Observation Env::get_observation()
{
    Observation obs{};

    obs.current_simulation_time = state.current_simulation_time;

    obs.current_cookies = state.current_cookies;

    obs.all_time_cookies = state.alltime_cookies;

    obs.handmade_cookies = state.handmade_cookies;

    obs.total_cps = economySystem.calculateEffectiveCPS(state, true);

    obs.has_seen_golden_cookie =
        state.has_seen_golden_cookie;

    obs.seconds_since_last_golden_cookie =
        state.has_seen_golden_cookie
            ? std::max(
                  0.0,
                  state.current_simulation_time -
                      state.last_golden_cookie_timestamp)
            : state.current_simulation_time;

    const bool episode_ongoing =
        state.alltime_cookies < Config::target_cookies &&
        state.current_simulation_time <
            Config::episode_length;

    if (episode_ongoing)
    {

        obs.valid_action_mask[0] = true;
    }

    obs.buildings_owned = state.buildingsOwned;

    // BUILDING affordability
    for (int building_index = 0; building_index < +BuildingType::BUILDING_COUNT; ++building_index)
    {
        const std::size_t index = static_cast<std::size_t>(building_index);

        obs.can_buy_1[index] = buildingSystem.canBuy(state, building_index, 1);

        obs.can_buy_10[index] = buildingSystem.canBuy(state, building_index, 10);

        obs.can_buy_100[index] = buildingSystem.canBuy(state, building_index, 100);

        if (episode_ongoing)
        {
            const int first_action =
                1 +
                building_index *
                    purchaseQuantityCount;

            obs.valid_action_mask[static_cast<std::size_t>(
                first_action)] = obs.can_buy_1[index];

            obs.valid_action_mask[static_cast<std::size_t>(
                first_action + 1)] = obs.can_buy_10[index];

            obs.valid_action_mask[static_cast<std::size_t>(
                first_action + 2)] = obs.can_buy_100[index];
        }
    }

    // UPGRADES
    for (int upgrade_index = 0; upgrade_index < +UpgradeType::UPGRADE_COUNT; ++upgrade_index)
    {
        const std::size_t index = static_cast<std::size_t>(upgrade_index);

        obs.upgrades_owned[index] = state.upgradesOwned[index];

        obs.upgrades_unlocked[index] = upgradeSystem.isUnlocked(state, upgrade_index);

        obs.can_buy_upgrades[index] = upgradeSystem.canBuy(state, upgrade_index);

        if (episode_ongoing)
        {
            obs.valid_action_mask[static_cast<std::size_t>(
                upgradeActionOffset +
                upgrade_index)] =
                obs.can_buy_upgrades[index];
        }
    }

    // ACTIVE BUFFS with REMAININIG DURATIONS
    for (int buff_index = 0; buff_index < +GoldenCookieBuff::GOLDEN_COOKIE_BUFF_COUNT; ++buff_index)
    {
        const GoldenCookieBuff buff_type = static_cast<GoldenCookieBuff>(buff_index);

        const auto active_buff = std::find_if(
            state.activeGoldenCookieBuffs.begin(),
            state.activeGoldenCookieBuffs.end(),
            [buff_type](const ActiveGoldenCookieBuff &buff)
            {
                return buff.buff_type == buff_type;
            });

        if (active_buff == state.activeGoldenCookieBuffs.end())
        {
            continue;
        }

        const double seconds_remaining = std::max(0.0, active_buff->expires_at - state.current_simulation_time);

#ifndef NDEBUG
        assert(std::isfinite(seconds_remaining));

        assert(seconds_remaining >= 0.0);
#endif

        obs.activeGoldenCookieBuffs.push_back(buff_type);

        obs.activeGoldenCookieBuffSecondsRemaining.push_back(seconds_remaining);
    }

#ifndef NDEBUG
    assert(std::isfinite(
        obs.seconds_since_last_golden_cookie));
    assert(
        obs.seconds_since_last_golden_cookie >= 0.0);

    assert(obs.activeGoldenCookieBuffs.size() == obs.activeGoldenCookieBuffSecondsRemaining.size());
#endif

    return obs;
}

bool Env::is_terminal()
{
    return classifyEpisode(state).outcome !=
           EpisodeOutcome::Ongoing;
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
        state.total_cps,
        state.current_simulation_time,
        state.buildingsOwned[+BuildingType::CURSOR],
        state.buildingsOwned[+BuildingType::GRANDMA],
        state.buildingsOwned[+BuildingType::FARM]);
}

unsigned int Env::getEpisodeSeed()
{
    return eventSystem.getEpisodeSeed();
}
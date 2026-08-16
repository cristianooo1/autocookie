#include "env.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cassert>

#ifndef NDEBUG
namespace
{
    void assertValidDecisionState(const GameState &state)
    {
        assert(std::isfinite(state.current_simulation_time));
        assert(std::isfinite(state.current_cookies));
        assert(std::isfinite(state.alltime_cookies));
        assert(std::isfinite(state.total_cps));
        assert(std::isfinite(state.cookies_per_click));

        assert(state.current_simulation_time >= 0.0);
        assert(state.current_simulation_time <= Config::episode_length);
        assert(state.current_cookies >= 0.0);
        assert(state.alltime_cookies >= 0.0);
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

            if (state.current_simulation_time <
                Config::episode_length)
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

    if (is_terminal())
    {
        return StepResult{
            .obs = get_observation(),
            .reward = 0.0,
            .done = true,
        };
    }

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
        bool clicking = true;
        double rate = economySystem.calculateEffectiveCPS(state, clicking);

        // get closest future wakeup events = building affordable OR golden cookie spawn OR buff expiration
        std::vector<NextScheduledEvent> future_events = simulationSystem.getTimeNextDecision(
            state,
            buildingSystem,
            eventSystem,
            rate);
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
                WakeUpDecisionEventType::EpisodeBoundary))
        {
            state.current_simulation_time =
                Config::episode_length;

            episode_ended = true;
        }
        else
        {
            process_events_at_current_time(future_events);
        }
    }
    else if (action.type == ActionType::BuyBuilding)
    {
        // START = T1
        PurchaseIntent purchase_intention = buildingSystem.validatePurchase(state, action);

        bool clicking = false;
        bool has_purchase_completed = false;
        double purchase_end_timestamp = state.current_simulation_time + Config::buying_time_cost;

#ifndef NDEBUG
        assert(std::isfinite(purchase_end_timestamp));
        assert(
            purchase_end_timestamp >
            state.current_simulation_time);
#endif

        // get closest future wakeup event/s = building affordable AND/OR golden cookie spawn AND/OR buff expiration
        while (state.current_simulation_time < purchase_end_timestamp)
        {
            // t1 -> event.timestamp -> t2 = buy_building

            std::vector<NextScheduledEvent> future_events = simulationSystem.getTimeNextInternalEvents(state, eventSystem);

            double internal_timestamp = future_events.at(0).absolute_timestamp;
#ifndef NDEBUG
            assert(!future_events.empty());
            assert(std::isfinite(internal_timestamp));
            assert(
                internal_timestamp >
                state.current_simulation_time);
#endif
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

            if (is_internal_timestamp_reached)
            {
                if (is_event_here(future_events, WakeUpDecisionEventType::EpisodeBoundary))
                {
                    state.current_simulation_time = Config::episode_length;
                    episode_ended = true;
                    break;
                }

                // at new timestamp != episode_finish, order is: expiration -> purchase_complete -> new spawn
                remove_expired_buffs_at_current_time(future_events);

                if (next_timestamp == purchase_end_timestamp &&
                    purchase_intention.canAfford)
                {
                    buildingSystem.makePurchase(state, purchase_intention);
                    has_purchase_completed = true;
                }

                process_golden_cookie_spawn_at_current_time(future_events);
            }

            if (state.current_simulation_time >= purchase_end_timestamp)
            {
                break;
            }
        }

        if (!episode_ended &&
            purchase_intention.canAfford &&
            !has_purchase_completed)
        {
            // T2
            buildingSystem.makePurchase(state, purchase_intention);
        }

#ifndef NDEBUG
        if (!episode_ended)
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

    return StepResult{
        .obs = get_observation(),
        .reward = get_reward(),
        .done = is_terminal() || episode_ended,
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

    this->prev_progress_alltime_cookies = state.alltime_cookies;
    this->prev_progress_cps = state.total_cps;

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
    obs.current_cookies = state.current_cookies,
    obs.all_time_cookies = state.alltime_cookies,
    obs.total_cps = economySystem.calculateEffectiveCPS(state, true);
    obs.buildings_owned = state.buildingsOwned;

    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        obs.can_buy_1[i] = buildingSystem.canBuy(state, i, 1);
        obs.can_buy_10[i] = buildingSystem.canBuy(state, i, 10);
        obs.can_buy_100[i] = buildingSystem.canBuy(state, i, 100);
    }

    for (int i = 0; i < +GoldenCookieBuff::GOLDEN_COOKIE_BUFF_COUNT; i++)
    {
        auto buff_type = static_cast<GoldenCookieBuff>(i);
        bool is_active = std::any_of(
            state.activeGoldenCookieBuffs.begin(),
            state.activeGoldenCookieBuffs.end(),
            [buff_type](const ActiveGoldenCookieBuff &buff)
            {
                return buff.buff_type == buff_type;
            });

        if (is_active)
        {
            obs.activeGoldenCookieBuffs.push_back(buff_type);
        }
    }
    return obs;
}

double Env::get_reward()
{
    double current_cps = economySystem.calculateEffectiveCPS(state, true);
    double reward = (state.alltime_cookies - prev_progress_alltime_cookies) + (current_cps - prev_progress_cps) * 5.0;

#ifndef NDEBUG
    assert(std::isfinite(current_cps));
    assert(std::isfinite(reward));
#endif

    prev_progress_alltime_cookies = state.alltime_cookies;
    prev_progress_cps = current_cps;

    return reward;
}

bool Env::is_terminal()
{
    if (state.current_simulation_time >= Config::episode_length)
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

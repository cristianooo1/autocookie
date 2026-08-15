#include "env.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Env::Env()
{
}

StepResult Env::step(const Action &action)
{
    constexpr double time_epsilon = 1e-9;

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
    HELPER FUNCTION
    processes all events at current gamestate in a specific order:
    1. remove expired buffs
    2. add new buffs
    */
    auto process_events_at_current_time =
        [&](const std::vector<NextScheduledEvent> &events,
            const bool clicking)
    {
        // 1. removed expired buffs
        if (is_event_here(
                events,
                WakeUpDecisionEventType::
                    GoldenCookieBuffExpiration))
        {
            eventSystem.removeGoldenCookieBuff(state);
        }

        // 2. add new buffs
        // LUCKY NEEDS RATE AFTER ALL BUFFS EXPIRE!!!!!!!!!!!
        if (is_event_here(
                events,
                WakeUpDecisionEventType::
                    GoldenCookieSpawned))
        {
            double rate =
                economySystem.calculateEffectiveCPS(
                    state,
                    clicking);

            eventSystem.processGoldenCookieBuff(
                state,
                rate);
        }
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
            process_events_at_current_time(
                future_events,
                clicking);
        }
    }
    else if (action.type == ActionType::BuyBuilding)
    {
        // START = T1
        PurchaseIntent purchase_intention = buildingSystem.validatePurchase(state, action);

        if (!purchase_intention.canAfford)
        {
            state.total_cps = economySystem.calculateEffectiveCPS(state, true);

            return StepResult{
                .obs = get_observation(),
                .reward = get_reward(),
                .done = false,
            };
        }

        bool clicking = false;
        double purchase_end_timestamp = state.current_simulation_time + Config::buying_time_cost;

        // get closest future wakeup event/s = building affordable AND/OR golden cookie spawn AND/OR buff expiration
        while (state.current_simulation_time + time_epsilon < purchase_end_timestamp)
        {
            // t1 -> event.timestamp -> t2 = buy_building

            std::vector<NextScheduledEvent> future_events = simulationSystem.getTimeNextInternalEvents(state, eventSystem);

            double internal_timestamp = future_events.at(0).absolute_timestamp;
            double next_timestamp = std::min(purchase_end_timestamp, internal_timestamp);

            // update economy with clicking=false t1 -> event.timestamp
            double dt = next_timestamp - state.current_simulation_time;
            economySystem.integrateOverDT(state, dt, clicking);

            // advance clock to event.timestamp
            state.current_simulation_time = next_timestamp;

            // process all events at event.timestamp
            bool is_internal_timestamp_reached = false;
            if (internal_timestamp <= next_timestamp + time_epsilon)
            {
                is_internal_timestamp_reached = true;
            }

            if (is_internal_timestamp_reached)
            {
                if (is_event_here(future_events, WakeUpDecisionEventType::EpisodeBoundary))
                {
                    state.current_simulation_time = Config::episode_length;
                    episode_ended = true;
                    break;
                }

                process_events_at_current_time(future_events, clicking);
            }

            if (state.current_simulation_time + time_epsilon >= purchase_end_timestamp)
            {
                break;
            }
        }

        if (!episode_ended)
        {
            // T2
            buildingSystem.makePurchase(state, purchase_intention);
        }
    }
    else
    {
        throw std::invalid_argument("UNKNOW ACTION!!!!!!!!!!!!!!!!!!!!");
    }

    state.total_cps = economySystem.calculateEffectiveCPS(state, true);

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

    return get_observation();
}

Observation Env::get_observation()
{
    Observation obs{};

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

    obs.activeGoldenCookieBuffs = state.activeGoldenCookieBuffs;
    return obs;
}

double Env::get_reward()
{
    double current_cps = economySystem.calculateEffectiveCPS(state, true);
    double reward = (state.alltime_cookies - prev_progress_alltime_cookies) + (current_cps - prev_progress_cps) * 5.0;

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

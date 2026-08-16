#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>
#include <cassert>

#include "simulationSystem.hpp"

#ifndef NDEBUG
namespace
{
    void assertValidFutureSchedule(
        const GameState &state,
        const std::vector<NextScheduledEvent> &events)
    {
        assert(!events.empty());

        for (const NextScheduledEvent &event : events)
        {
            const int event_type =
                static_cast<int>(event.event_type);

            assert(event_type >= 0);
            assert(
                event_type <
                +WakeUpDecisionEventType::EVENT_COUNT);

            assert(std::isfinite(event.absolute_timestamp));

            // These functions schedule future events from a fully processed
            // non-terminal decision state.
            assert(
                event.absolute_timestamp >
                state.current_simulation_time);
        }
    }
}
#endif

SimulationSystem::SimulationSystem()
{
}

std::vector<NextScheduledEvent> SimulationSystem::getTimeNextDecision(
    GameState &state,
    BuildingSystem &building_system,
    EventSystem &event_system,
    double rate)
{
    std::vector<NextScheduledEvent> future_scheduled_events{};

    double next_affordable_building = std::numeric_limits<double>::infinity();

    constexpr std::array<int, 3> quantities{1, 10, 100};

    for (int i = 0; i < +BuildingType::BUILDING_COUNT; i++)
    {
        for (const int quantity : quantities)
        {
            double timestamp = building_system.getAbsoluteTimestampNextAffordableBuilding(
                state,
                i,
                quantity,
                rate);

            next_affordable_building = std::min(next_affordable_building, timestamp);
        }
    }
    if (std::isfinite(next_affordable_building))
    {
        future_scheduled_events.push_back(NextScheduledEvent{
            .event_type = WakeUpDecisionEventType::BuildingAvailable,
            .absolute_timestamp = next_affordable_building});
    }

    future_scheduled_events.push_back(NextScheduledEvent{
        .event_type = WakeUpDecisionEventType::GoldenCookieSpawned,
        .absolute_timestamp = event_system.getNextGoldenCookieSpawn(),
    });

    if (state.activeGoldenCookieBuffs.size() > 0)
    {
        double next_expiration = std::numeric_limits<double>::infinity();

        for (const ActiveGoldenCookieBuff &buff : state.activeGoldenCookieBuffs)
        {
            next_expiration = std::min(next_expiration, buff.expires_at);
        }

        future_scheduled_events.push_back(NextScheduledEvent{
            .event_type = WakeUpDecisionEventType::GoldenCookieBuffExpiration,
            .absolute_timestamp = next_expiration});
    }

    future_scheduled_events.push_back(NextScheduledEvent{
        .event_type = WakeUpDecisionEventType::EpisodeBoundary,
        .absolute_timestamp = Config::episode_length});

#ifndef NDEBUG
    assertValidFutureSchedule(
        state,
        future_scheduled_events);
#endif

    return this->getEarliestEvent(future_scheduled_events);
}

std::vector<NextScheduledEvent> SimulationSystem::getTimeNextInternalEvents(
    GameState &state,
    EventSystem &event_system)
{
    std::vector<NextScheduledEvent> future_events;

    future_events.push_back(NextScheduledEvent{
        .event_type = WakeUpDecisionEventType::GoldenCookieSpawned,
        .absolute_timestamp = event_system.getNextGoldenCookieSpawn()});

    if (state.activeGoldenCookieBuffs.size() > 0)
    {
        double next_expiration = std::numeric_limits<double>::infinity();

        for (const ActiveGoldenCookieBuff &buff : state.activeGoldenCookieBuffs)
        {
            next_expiration = std::min(next_expiration, buff.expires_at);
        }

        future_events.push_back(NextScheduledEvent{
            .event_type = WakeUpDecisionEventType::GoldenCookieBuffExpiration,
            .absolute_timestamp = next_expiration});
    }

    future_events.push_back(NextScheduledEvent{
        .event_type = WakeUpDecisionEventType::EpisodeBoundary,
        .absolute_timestamp = Config::episode_length});

#ifndef NDEBUG
    assertValidFutureSchedule(
        state,
        future_events);
#endif

    return this->getEarliestEvent(future_events);
}

std::vector<NextScheduledEvent> SimulationSystem::getEarliestEvent(const std::vector<NextScheduledEvent> &future_events)
{
#ifndef NDEBUG
    assert(!future_events.empty());

    for (const NextScheduledEvent &event : future_events)
    {
        assert(std::isfinite(event.absolute_timestamp));
    }
#endif

    double earliest_event = std::numeric_limits<double>::infinity();

    for (const NextScheduledEvent &event : future_events)
    {
        earliest_event = std::min(earliest_event, event.absolute_timestamp);
    }

    std::vector<NextScheduledEvent> earliest_events;

    for (const NextScheduledEvent &event : future_events)
    {
        if (event.absolute_timestamp == earliest_event)
        {
            earliest_events.push_back(event);
        }
    }

#ifndef NDEBUG
    assert(!earliest_events.empty());

    for (const NextScheduledEvent &event : earliest_events)
    {
        assert(event.absolute_timestamp == earliest_event);
    }
#endif

    return earliest_events;
}
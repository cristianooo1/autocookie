#pragma once

#include <vector>

#include "gameState.hpp"
#include "config.hpp"
#include "types.hpp"
#include "buildingSystem.hpp"
#include "eventSystem.hpp"
#include "economySystem.hpp"
#include "upgradeSystem.hpp"

#include "eventTypes.hpp"

class SimulationSystem
{
public:
    SimulationSystem();

    std::vector<NextScheduledEvent> getTimeNextDecision(
        GameState &state,
        BuildingSystem &building_system,
        UpgradeSystem &upgrade_system,
        EventSystem &event_system,
        double rate,
        double clicking_rate = 0.0);

    std::vector<NextScheduledEvent> getTimeNextInternalEvents(
        GameState &state,
        EventSystem &event_system,
        double rate);

    std::vector<NextScheduledEvent> getEarliestEvent(const std::vector<NextScheduledEvent> &future_events);

private:
};

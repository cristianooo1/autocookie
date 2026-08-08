#pragma once

#include <cmath>

#include "gameState.hpp"
#include "config.hpp"
#include "types.hpp"

class BuildingSystem
{
public:
    BuildingSystem();

    void update(GameState &state, const Action &action);

    bool can_buy(const GameState &state, const int buildingIndex, const int quantity);

    double time_until_affordable(GameState &state, const int buildingIndex);

private:
};
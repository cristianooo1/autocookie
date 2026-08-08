#pragma once

#include "gameState.hpp"
#include "config.hpp"
#include "types.hpp"

class Economy
{
public:
    void update(GameState &state, const double dt);
};
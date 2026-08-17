#pragma once

#include "gameState.hpp"
#include "config.hpp"
#include "types.hpp"

class EconomySystem
{
public:
    double calculateEffectiveCPS(const GameState &state, const bool is_clicking_active);

    void integrateOverDT(GameState &state, const double dt, const bool is_clicking_active);

    double calculateCookiesPerClick(const GameState &state);
};
#pragma once

#include "gameState.hpp"
#include "config.hpp"
#include "types.hpp"

class BuildingSystem
{
public:
    BuildingSystem();

    double calculateTotalPrice(
        const GameState &state,
        int buildingIndex,
        int quantity);

    PurchaseIntent validatePurchase(const GameState &state, const Action &action);

    void makePurchase(GameState &state, const PurchaseIntent &purchase);

    bool canBuy(const GameState &state, int buildingIndex, int quantity);

    double getAbsoluteTimestampNextAffordableBuilding(
        const GameState &state,
        int buildingIndex,
        int quantity,
        double rate);

private:
};
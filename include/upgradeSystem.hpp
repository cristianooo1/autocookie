#pragma once

#include <array>

#include "gameState.hpp"
#include "types.hpp"

struct UpgradeDefinition
{
    double price{0.0};
    BuildingType requiredBuilding{};
    int requiredBuildingCount{0};
};

inline constexpr std::array<UpgradeDefinition, +UpgradeType::UPGRADE_COUNT> upgradeDefinitions{
    // CURSOR UPGRADES
    UpgradeDefinition{
        .price = 100.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        .price = 500.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        .price = 10000.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 10,
    },
    UpgradeDefinition{
        .price = 100000.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 25,
    },

    // GRANDMA UPGRADES
    UpgradeDefinition{
        .price = 1000.0,
        .requiredBuilding = BuildingType::GRANDMA,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        .price = 5000.0,
        .requiredBuilding = BuildingType::GRANDMA,
        .requiredBuildingCount = 5,
    },
    UpgradeDefinition{
        .price = 50000.0,
        .requiredBuilding = BuildingType::GRANDMA,
        .requiredBuildingCount = 25,
    },

    // FARM UPGRADES
    UpgradeDefinition{
        .price = 11000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        .price = 55000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 5,
    },
    UpgradeDefinition{
        .price = 550000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 25,
    },

    // MINE UPGRADES
    UpgradeDefinition{
        .price = 120000.0,
        .requiredBuilding = BuildingType::MINE,
        .requiredBuildingCount = 1,
    }};

class UpgradeSystem
{
public:
    bool isUnlocked(const GameState &state, int upgradeIndex) const;

    bool canBuy(const GameState &state, int upgradeIndex) const;

    UpgradePurchaseIntent validatePurchase(
        const GameState &state,
        const Action &action) const;

    void makePurchase(
        GameState &state,
        const UpgradePurchaseIntent &purchase) const;

    double getAbsoluteTimestampNextAffordableUpgrade(
        const GameState &state,
        int upgradeIndex,
        double rate) const;
};
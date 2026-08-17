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
        // REINFORCED INDEX FINGER
        .price = 100.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        // CARPAL TUNNEL PREVENTION CREAM
        .price = 500.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        // AMBIDEXTROUS
        .price = 10000.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 10,
    },
    UpgradeDefinition{
        // THOUSAND FINGERS
        .price = 100000.0,
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 25,
    },
    UpgradeDefinition{
        // PLASTIC MOUSE
        .price = 50000.0,
        // real requirement handled by isUnlocked()
        .requiredBuilding = BuildingType::CURSOR,
        .requiredBuildingCount = 0,
    },

    // GRANDMA UPGRADES
    UpgradeDefinition{
        // FORWARDS FROM GRANDMA
        .price = 1000.0,
        .requiredBuilding = BuildingType::GRANDMA,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        // STEELPLATED ROLLING PINS
        .price = 5000.0,
        .requiredBuilding = BuildingType::GRANDMA,
        .requiredBuildingCount = 5,
    },
    UpgradeDefinition{
        // LUBRICATED DENTURES
        .price = 50000.0,
        .requiredBuilding = BuildingType::GRANDMA,
        .requiredBuildingCount = 25,
    },
    UpgradeDefinition{
        // FARMER GRANDMAS
        .price = 55000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 15,
    },

    // FARM UPGRADES
    UpgradeDefinition{
        // CHEAP HOES
        .price = 11000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        // FERTILIZER
        .price = 55000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 5,
    },
    UpgradeDefinition{
        // COOKIE TREES
        .price = 550000.0,
        .requiredBuilding = BuildingType::FARM,
        .requiredBuildingCount = 25,
    },

    // MINE UPGRADES
    UpgradeDefinition{
        // SUGAR GAS
        .price = 120000.0,
        .requiredBuilding = BuildingType::MINE,
        .requiredBuildingCount = 1,
    },
    UpgradeDefinition{
        // MEGADRILL
        .price = 600000.0,
        .requiredBuilding = BuildingType::MINE,
        .requiredBuildingCount = 5,
    },
};

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
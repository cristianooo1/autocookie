#pragma once

#include <array>
#include <stdexcept>
#include "config.hpp"

enum class BuildingType
{
    CURSOR = 0,
    GRANDMA,
    FARM,
    MINE,
    FACTORY,

    // building count for all buildings available before 1mil cookies
    BUILDING_COUNT,
};

struct BuildingDefinition
{
    double base_cost{0.1};
    double base_cps{0.1};
};

inline constexpr std::array<BuildingDefinition, static_cast<int>(BuildingType::BUILDING_COUNT)> buildingsDefinitions{
    BuildingDefinition{
        // CURSOR = 0
        .base_cost = 15.0,
        .base_cps = 0.1,
    },
    BuildingDefinition{
        // GRANDMA = 1
        .base_cost = 100.0,
        .base_cps = 1.0,
    },
    BuildingDefinition{
        // FARM = 2
        .base_cost = 1100.0,
        .base_cps = 8.0,
    },
    BuildingDefinition{
        // MINE = 3
        .base_cost = 12'000.0,
        .base_cps = 47.0,
    },
    BuildingDefinition{
        // FACTORY = 4
        .base_cost = 130'000.0,
        .base_cps = 260.0,
    }};

enum class BuyingQuantity
{
    ONE = 1,
    TEN = 10,
    ONE_HUNDRED = 100,
};

inline constexpr std::array<BuyingQuantity, 3>
    buyingQuantities{
        BuyingQuantity::ONE,
        BuyingQuantity::TEN,
        BuyingQuantity::ONE_HUNDRED,
    };

inline constexpr int purchaseQuantityCount{static_cast<int>(buyingQuantities.size())};

inline constexpr int buildingPurchaseActionCount{static_cast<int>(BuildingType::BUILDING_COUNT) * purchaseQuantityCount};

enum class UpgradeType
{
    /*
    THESE ARE THE ONLY UPGRADES THAT CAN BE PURCHASED IN THE 1MIL COOKIE SCENARIO
    THERE ARE STILL SOME THAT ARE NOT IMPLEMENTED
    */

    // CURSOR
    REINFORCED_INDEX_FINGER = 0,
    CARPAL_TUNNEL_PREVENTION_CREAM,
    AMBIDEXTROUS,
    THOUSAND_FINGERS,

    // GRANDMA
    FORWARDS_FROM_GRANDMA,
    STEEL_PLATED_ROLLING_PINS,
    LUBRICATED_DENTURES,

    // FARM
    CHEAP_HOES,
    FERTILIZER,
    COOKIE_TREES,

    // MINE
    SUGAR_GAS,

    UPGRADE_COUNT,
};

enum class ActionType
{
    Advance,
    BuyBuilding,
    BuyUpgrade,
};

struct Action
{
    ActionType type{};
    BuildingType buildingIndex{};
    BuyingQuantity quantity{};
    UpgradeType upgradeIndex{};
};

struct PurchaseIntent
{
    bool canAfford{false};
    BuildingType buildingIndex{};
    BuyingQuantity quantity{};
    double totalPrice{0.0};
};

struct UpgradePurchaseIntent
{
    bool canPurchase{false};
    UpgradeType upgradeIndex{};
    double price{0.0};
};

/*
DISCRETE ACTION LAYOUT
NECESSARY FOR RL TRAINING

1 advance
15 building purchases
11 upgrade purchases
27 total actions

*/

inline constexpr int upgradeActionOffset{
    1 + buildingPurchaseActionCount};

inline constexpr int discreteActionCount{
    upgradeActionOffset +
    static_cast<int>(UpgradeType::UPGRADE_COUNT)};

inline Action actionFromDiscreteIndex(
    const int action_index)
{
    if (action_index < 0 ||
        action_index >= discreteActionCount)
    {
        throw std::out_of_range(
            "DISCRETE ACTION INDEX OUT OF RANGE");
    }

    if (action_index == 0)
    {
        return Action{
            .type = ActionType::Advance,
        };
    }

    if (action_index < upgradeActionOffset)
    {
        const int purchase_index =
            action_index - 1;

        const int building_index =
            purchase_index /
            purchaseQuantityCount;

        const int quantity_index =
            purchase_index %
            purchaseQuantityCount;

        return Action{
            .type = ActionType::BuyBuilding,
            .buildingIndex =
                static_cast<BuildingType>(
                    building_index),
            .quantity =
                buyingQuantities[static_cast<std::size_t>(
                    quantity_index)],
        };
    }

    return Action{
        .type = ActionType::BuyUpgrade,
        .upgradeIndex =
            static_cast<UpgradeType>(
                action_index -
                upgradeActionOffset),
    };
}
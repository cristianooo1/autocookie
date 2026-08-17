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

enum class ActionType
{
    Advance,
    BuyBuilding,
};

struct Action
{
    ActionType type{};
    BuildingType buildingIndex{};
    BuyingQuantity quantity{};
};

struct PurchaseIntent
{
    bool canAfford{false};
    BuildingType buildingIndex{};
    BuyingQuantity quantity{};
    double totalPrice{0.0};
};

/*
DISCRETE ACTION LAYOUT
NECESSARY FOR RL TRAINING

0       Advance
1-3     Cursor x1, x10, x100
4-6     Grandma x1, x10, x100
7-9     Farm x1, x10, x100
10-12   Mine x1, x10, x100
13-15   Factory x1, x10, x100

*/

inline constexpr std::array<BuyingQuantity, 3> buyingQuantities{
    BuyingQuantity::ONE,
    BuyingQuantity::TEN,
    BuyingQuantity::ONE_HUNDRED,
};

inline constexpr int purchaseQuantityCount{static_cast<int>(buyingQuantities.size())};

inline constexpr int discreteActionCount{1 + +BuildingType::BUILDING_COUNT * purchaseQuantityCount};

inline Action actionFromDiscreteIndex(const int action_index)
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

    const int purchase_index = action_index - 1;

    const int building_index =
        purchase_index / purchaseQuantityCount;

    const int quantity_index =
        purchase_index % purchaseQuantityCount;

    return Action{
        .type = ActionType::BuyBuilding,
        .buildingIndex = static_cast<BuildingType>(building_index),
        .quantity = buyingQuantities[static_cast<std::size_t>(quantity_index)],
    };
}
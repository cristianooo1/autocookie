#pragma once

enum class BuildingType
{
    CURSOR = 0,
    GRANDMA,
    FARM,
    BUILDING_COUNT,
};

struct BuildingDefinition
{
    double base_cost{0.1};
    double base_cps{0.1};
};

constexpr std::array<BuildingDefinition, static_cast<int>(BuildingType::BUILDING_COUNT)> buildingsDefinitions{
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
    }};

enum class BuyingQuantity
{
    ONE = 1,
    TEN = 10,
    ONE_HUNDRED = 100,
};

enum class ActionType
{
    // ClickCookie = 0,  REMOVE THIS from the other parts !!!!!!!!!!!!!!
    Advance,
    BuyBuilding,
    // BuyUpgrade
    // ClickGoldenCookie,
    // Ascend,

};

struct Action
{
    ActionType type{};
    BuildingType buildingIndex{};
    BuyingQuantity quantity{};
};

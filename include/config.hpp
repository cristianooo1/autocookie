#pragma once

#include <array>

namespace Config
{
    struct BuildingDefinition
    {
        double base_cost{0.1};
        double base_production{0.1};
        double cost_multiplier{0.1};
    };

    constexpr int TOTAL_NR_BUILDINGS_AVAILABLE = 3;

    constexpr std::array<BuildingDefinition, TOTAL_NR_BUILDINGS_AVAILABLE> buildingsDefinitions{
        BuildingDefinition{
            // CURSOR = 0
            .base_cost = 1,
            .base_production = 0.1,
            .cost_multiplier = 1.15,
        },
        BuildingDefinition{
            // GRANDMA = 1
            .base_cost = 20,
            .base_production = 0.25,
            .cost_multiplier = 1.25,
        },
        BuildingDefinition{
            // FARM = 2
            .base_cost = 50,
            .base_production = 0.4,
            .cost_multiplier = 1.5,
        }};
};
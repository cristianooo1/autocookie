#include "buildingSystem.hpp"

BuildingSystem::BuildingSystem()
{
}

void BuildingSystem::update(GameState &state, const Action &action)
{

    /*
    math is taken from here:
    https://cookieclicker.fandom.com/wiki/Building
    */

    if (action.type == ActionType::BuyBuilding)
    {

        if (can_buy(state, +action.buildingIndex, +action.quantity))
        {
            double price = buildingsDefinitions[+action.buildingIndex].base_cost *
                           std::pow(
                               1.15,
                               static_cast<double>(state.buildingsOwned[+action.buildingIndex]));

            if (action.quantity == BuyingQuantity::ONE)
            {
                state.current_cookies -= price;
                state.buildingsOwned[+action.buildingIndex] += 1;
            }

            else if (action.quantity == BuyingQuantity::TEN)
            {
                double price_10 = price * Config::building_price_multiplier_buy_10;

                if (state.current_cookies >= price_10)
                {
                    state.current_cookies -= price_10;
                    state.buildingsOwned[+action.buildingIndex] += 10;
                }
            }

            else if (action.quantity == BuyingQuantity::ONE_HUNDRED)
            {
                double price_100 = price * Config::building_price_multiplier_buy_100;

                if (state.current_cookies >= price_100)
                {
                    state.current_cookies -= price_100;
                    state.buildingsOwned[+action.buildingIndex] += 100;
                }
            }
        }
    }
}

bool BuildingSystem::can_buy(const GameState &state, const int buildingIndex, const int quantity)
{

    double price = buildingsDefinitions[buildingIndex].base_cost *
                   std::pow(
                       1.15,
                       static_cast<double>(state.buildingsOwned[buildingIndex]));

    if (quantity == 1 && state.current_cookies >= price)
    {
        return true;
    }

    else if (quantity == 10)
    {
        double price_10 = price * Config::building_price_multiplier_buy_10;

        if (state.current_cookies >= price_10)
        {
            return true;
        }
    }

    else if (quantity == 100)
    {
        double price_100 = price * Config::building_price_multiplier_buy_100;

        if (state.current_cookies >= price_100)
        {
            return true;
        }
    }

    return false;
}

double BuildingSystem::time_until_affordable(GameState &state, const int buildingIndex)
{

    if (!can_buy(state, buildingIndex, 1))
    {
        double price = buildingsDefinitions[buildingIndex].base_cost *
                       std::pow(
                           1.15,
                           static_cast<double>(state.buildingsOwned[buildingIndex]));

        return ((price - state.current_cookies) / state.cps);
    }
    return 0.0;
}
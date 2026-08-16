#include "buildingSystem.hpp"

#include <cmath>
#include <stdexcept>
#include <limits>

namespace
{
    void validateBuildingIndex(int buildingIndex)
    {
        if (buildingIndex < 0 || buildingIndex >= +BuildingType::BUILDING_COUNT)
        {
            throw std::out_of_range("INVALID BUILDING INDEX");
        }
    }

    void validateQuantity(int quantity)
    {
        if (quantity != 1 && quantity != 10 && quantity != 100)
        {
            throw std::invalid_argument("INVALID QUANTITY! ONLY x1, x10, x100");
        }
    }
}

BuildingSystem::BuildingSystem()
{
}

double BuildingSystem::calculateTotalPrice(
    const GameState &state,
    int buildingIndex,
    int quantity)
{
    validateBuildingIndex(buildingIndex);
    validateQuantity(quantity);

    double first_price =
        buildingsDefinitions[buildingIndex].base_cost *
        std::pow(
            1.15,
            static_cast<double>(state.buildingsOwned[buildingIndex]));

    switch (quantity)
    {
    case 1:
        return first_price;

    case 10:
        return first_price *
               Config::building_price_multiplier_buy_10;

    case 100:
        return first_price *
               Config::building_price_multiplier_buy_100;

    default:
        throw std::invalid_argument("INVALID QUANTITY! ONLY x1, x10, x100");
    }
}

PurchaseIntent BuildingSystem::validatePurchase(
    const GameState &state,
    const Action &action)
{
    if (action.type != ActionType::BuyBuilding)
    {
        throw std::invalid_argument("PURCHASE REQUIRES BUY_BUILDING ACTIO!N!!!!!!!!!!!!!!!");
    }

    double price =
        calculateTotalPrice(state, +action.buildingIndex, +action.quantity);

    bool can_buy = state.current_cookies >= price;

    return PurchaseIntent{
        .canAfford = can_buy,
        .buildingIndex = action.buildingIndex,
        .quantity = action.quantity,
        .totalPrice = price};
}

void BuildingSystem::makePurchase(
    GameState &state,
    const PurchaseIntent &purchase)
{
    validateBuildingIndex(+purchase.buildingIndex);
    validateQuantity(+purchase.quantity);

    if (!purchase.canAfford)
    {
        throw std::logic_error("NOT ENOUGH COOKIES FOR PURCHASE");
    }

    if (state.current_cookies < purchase.totalPrice)
    {
        throw std::logic_error(
            "IN THE 200MS AN EVENT CAUSED A DROP IN CURRENT_COOKIES SO THE PURCHASE IS UNNAFORDABLE LOL");
    }

    state.current_cookies -= purchase.totalPrice;
    state.buildingsOwned[+purchase.buildingIndex] += +purchase.quantity;
}

bool BuildingSystem::canBuy(
    const GameState &state,
    int buildingIndex,
    int quantity)
{

    double price =
        calculateTotalPrice(state, buildingIndex, quantity);

    if (state.current_cookies >= price)
    {
        return true;
    }
    else
    {
        return false;
    }
}

double BuildingSystem::getAbsoluteTimestampNextAffordableBuilding(
    const GameState &state,
    int buildingIndex,
    int quantity,
    double rate)
{
    /*
    returns the absolute timestamp for when the SPECIFIED building by "buildingIndex" becomes affordable
    return INFINITY if that specified building is already affordable
    */

    double price =
        calculateTotalPrice(state, buildingIndex, quantity);

    if (state.current_cookies >= price || rate <= 0.0)
    {
        return std::numeric_limits<double>::infinity();
    }

    return state.current_simulation_time +
           (price - state.current_cookies) / rate;
}
#include "buildingSystem.hpp"

#include <cmath>
#include <stdexcept>
#include <limits>

#include <cassert>

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

    double raw_first_price = buildingsDefinitions[static_cast<std::size_t>(buildingIndex)].base_cost *
                             std::pow(
                                 Config::building_price_growth,
                                 static_cast<double>(state.buildingsOwned[static_cast<std::size_t>(buildingIndex)]));

#ifndef NDEBUG
    assert(std::isfinite(raw_first_price));
    assert(raw_first_price > 0.0);
#endif

    double raw_total_price = 0.0;

    switch (quantity)
    {
    case 1:
        raw_total_price = raw_first_price;
        break;

    case 10:
        raw_total_price =
            raw_first_price *
            Config::building_price_multiplier_buy_10;
        break;

    case 100:
        raw_total_price =
            raw_first_price *
            Config::building_price_multiplier_buy_100;
        break;

    default:
        throw std::invalid_argument(
            "INVALID QUANTITY! ONLY x1, x10, x100");
    }

    // round upwards
    double total_price = std::ceil(raw_total_price);

#ifndef NDEBUG
    assert(std::isfinite(total_price));
    assert(total_price > 0.0);
#endif

    return total_price;
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

#ifndef NDEBUG
    assert(std::isfinite(state.current_cookies));
    assert(std::isfinite(purchase.totalPrice));
    assert(purchase.totalPrice > 0.0);

    const int building_index =
        static_cast<int>(purchase.buildingIndex);

    const int quantity =
        static_cast<int>(purchase.quantity);

    const double cookies_before =
        state.current_cookies;

    const int buildings_before =
        state.buildingsOwned[static_cast<std::size_t>(building_index)];

    assert(buildings_before >= 0);
    assert(
        buildings_before <=
        std::numeric_limits<int>::max() - quantity);
#endif

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

#ifndef NDEBUG
    assert(std::isfinite(state.current_cookies));
    assert(state.current_cookies >= 0.0);

    assert(
        state.current_cookies ==
        cookies_before - purchase.totalPrice);

    assert(
        state.buildingsOwned[static_cast<std::size_t>(building_index)] ==
        buildings_before + quantity);
#endif
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

    double timestamp =
        state.current_simulation_time +
        (price - state.current_cookies) / rate;

    if (timestamp <= state.current_simulation_time)
    {
        return std::nextafter(
            state.current_simulation_time,
            std::numeric_limits<double>::infinity());
    }

    return timestamp;
}
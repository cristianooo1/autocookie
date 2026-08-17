#include "upgradeSystem.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
    void validateUpgradeIndex(const int upgradeIndex)
    {
        if (upgradeIndex < 0 ||
            upgradeIndex >= +UpgradeType::UPGRADE_COUNT)
        {
            throw std::out_of_range(
                "INVALID UPGRADE INDEX");
        }
    }
}

bool UpgradeSystem::isUnlocked(
    const GameState &state,
    const int upgradeIndex) const
{
    validateUpgradeIndex(upgradeIndex);

    const UpgradeType upgrade = static_cast<UpgradeType>(upgradeIndex);

    if (upgrade == UpgradeType::PLASTIC_MOUSE)
    {
        return state.handmade_cookies >= Config::plastic_mouse_unlock_cookies;
    }

    if (upgrade == UpgradeType::FARMER_GRANDMAS)
    {
        return state.buildingsOwned[+BuildingType::FARM] >= 15 && state.buildingsOwned[+BuildingType::GRANDMA] >= 1;
    }

    const UpgradeDefinition &definition = upgradeDefinitions[static_cast<std::size_t>(upgradeIndex)];

    return state.buildingsOwned[+definition.requiredBuilding] >= definition.requiredBuildingCount;
}

bool UpgradeSystem::canBuy(
    const GameState &state,
    const int upgradeIndex) const
{
    validateUpgradeIndex(upgradeIndex);

    if (!isUnlocked(state, upgradeIndex))
    {
        return false;
    }

    if (state.upgradesOwned[static_cast<std::size_t>(upgradeIndex)])
    {
        return false;
    }

    return state.current_cookies >= upgradeDefinitions[static_cast<std::size_t>(upgradeIndex)].price;
}

UpgradePurchaseIntent
UpgradeSystem::validatePurchase(
    const GameState &state,
    const Action &action) const
{
    if (action.type != ActionType::BuyUpgrade)
    {
        throw std::invalid_argument(
            "UPGRADE PURCHASE REQUIRES BUY_UPGRADE ACTION");
    }

    const int upgrade_index = +action.upgradeIndex;

    validateUpgradeIndex(upgrade_index);

    const double price = upgradeDefinitions[static_cast<std::size_t>(upgrade_index)].price;

    return UpgradePurchaseIntent{
        .canPurchase = canBuy(state, upgrade_index),
        .upgradeIndex = action.upgradeIndex,
        .price = price,
    };
}

void UpgradeSystem::makePurchase(
    GameState &state,
    const UpgradePurchaseIntent &purchase) const
{
    const int upgrade_index = +purchase.upgradeIndex;

    validateUpgradeIndex(upgrade_index);

    if (!purchase.canPurchase)
    {
        throw std::logic_error("UPGRADE WAS NOT PURCHASABLE AT ACTION START");
    }

    if (state.upgradesOwned[static_cast<std::size_t>(
            upgrade_index)])
    {
        throw std::logic_error(
            "UPGRADE ALREADY OWNED");
    }

    if (state.current_cookies <
        purchase.price)
    {
        throw std::logic_error(
            "UPGRADE NO LONGER AFFORDABLE "
            "AT PURCHASE COMPLETION");
    }

#ifndef NDEBUG
    const double cookies_before =
        state.current_cookies;
#endif

    state.current_cookies -= purchase.price;

    state.upgradesOwned[static_cast<std::size_t>(
        upgrade_index)] = true;

#ifndef NDEBUG
    assert(std::isfinite(state.current_cookies));
    assert(state.current_cookies >= 0.0);

    assert(
        state.current_cookies ==
        cookies_before - purchase.price);

    assert(
        state.upgradesOwned[static_cast<std::size_t>(
            upgrade_index)]);
#endif
}

double UpgradeSystem::
    getAbsoluteTimestampNextAffordableUpgrade(
        const GameState &state,
        const int upgradeIndex,
        const double rate) const
{
    validateUpgradeIndex(upgradeIndex);

    if (!isUnlocked(state, upgradeIndex) ||
        state.upgradesOwned[static_cast<std::size_t>(
            upgradeIndex)])
    {
        return std::numeric_limits<double>::
            infinity();
    }

    const double price =
        upgradeDefinitions[static_cast<std::size_t>(
                               upgradeIndex)]
            .price;

    if (state.current_cookies >= price ||
        rate <= 0.0)
    {
        return std::numeric_limits<double>::
            infinity();
    }

    double timestamp =
        state.current_simulation_time +
        (price - state.current_cookies) /
            rate;

    if (timestamp <=
        state.current_simulation_time)
    {
        timestamp = std::nextafter(
            state.current_simulation_time,
            std::numeric_limits<double>::
                infinity());
    }

    return timestamp;
}
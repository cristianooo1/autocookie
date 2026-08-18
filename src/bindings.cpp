#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/vector.h>

#include "config.hpp"
#include "env.hpp"
#include "eventTypes.hpp"
#include "types.hpp"
#include "upgradeSystem.hpp"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(autocookie, m)
{
    m.doc() = "COOKIE CLICKER RL ENVIRONMENT";

    nb::enum_<BuildingType>(m, "BuildingType")
        .value("CURSOR", BuildingType::CURSOR)
        .value("GRANDMA", BuildingType::GRANDMA)
        .value("FARM", BuildingType::FARM)
        .value("MINE", BuildingType::MINE)
        .value("FACTORY", BuildingType::FACTORY);

    nb::enum_<BuyingQuantity>(m, "BuyingQuantity")
        .value("ONE", BuyingQuantity::ONE)
        .value("TEN", BuyingQuantity::TEN)
        .value("ONE_HUNDRED", BuyingQuantity::ONE_HUNDRED);

    nb::enum_<UpgradeType>(m, "UpgradeType")
        .value(
            "REINFORCED_INDEX_FINGER",
            UpgradeType::REINFORCED_INDEX_FINGER)
        .value(
            "CARPAL_TUNNEL_PREVENTION_CREAM",
            UpgradeType::CARPAL_TUNNEL_PREVENTION_CREAM)
        .value("AMBIDEXTROUS", UpgradeType::AMBIDEXTROUS)
        .value("THOUSAND_FINGERS", UpgradeType::THOUSAND_FINGERS)
        .value("PLASTIC_MOUSE", UpgradeType::PLASTIC_MOUSE)
        .value(
            "FORWARDS_FROM_GRANDMA",
            UpgradeType::FORWARDS_FROM_GRANDMA)
        .value(
            "STEEL_PLATED_ROLLING_PINS",
            UpgradeType::STEEL_PLATED_ROLLING_PINS)
        .value(
            "LUBRICATED_DENTURES",
            UpgradeType::LUBRICATED_DENTURES)
        .value("FARMER_GRANDMAS", UpgradeType::FARMER_GRANDMAS)
        .value("CHEAP_HOES", UpgradeType::CHEAP_HOES)
        .value("FERTILIZER", UpgradeType::FERTILIZER)
        .value("COOKIE_TREES", UpgradeType::COOKIE_TREES)
        .value("SUGAR_GAS", UpgradeType::SUGAR_GAS)
        .value("MEGADRILL", UpgradeType::MEGADRILL);

    nb::enum_<ActionType>(m, "ActionType")
        .value("Advance", ActionType::Advance)
        .value("BuyBuilding", ActionType::BuyBuilding)
        .value("BuyUpgrade", ActionType::BuyUpgrade);

    nb::enum_<GoldenCookieBuff>(m, "GoldenCookieBuff")
        .value("LUCKY", GoldenCookieBuff::LUCKY)
        .value("FRENZY", GoldenCookieBuff::FRENZY)
        .value("BUILDING_SPECIAL", GoldenCookieBuff::BUILDING_SPECIAL)
        .value("DRAGON_HARVEST", GoldenCookieBuff::DRAGON_HARVEST)
        .value("DRAGON_FLIGHT", GoldenCookieBuff::DRAGON_FLIGHT)
        .value("CLICK_FRENZY", GoldenCookieBuff::CLICK_FRENZY)
        .value(
            "EVERYTHING_MUST_GO",
            GoldenCookieBuff::EVERYTHING_MUST_GO)
        .value("BLAB", GoldenCookieBuff::BLAB);

    nb::enum_<Config::RewardMode>(m, "RewardMode")
        .value("TimeSuccess", Config::RewardMode::TimeSuccess)
        .value(
            "TimeSuccessLogPotential",
            Config::RewardMode::TimeSuccessLogPotential)
        .value(
            "OriginalCookiesPlusCps",
            Config::RewardMode::OriginalCookiesPlusCps);

    nb::class_<Action>(m, "Action")
        .def(nb::init<>())
        .def_rw("type", &Action::type)
        .def_rw("building_index", &Action::buildingIndex)
        .def_rw("quantity", &Action::quantity)
        .def_rw("upgrade_index", &Action::upgradeIndex);

    nb::class_<BuildingDefinition>(m, "BuildingDefinition")
        .def_ro("base_cost", &BuildingDefinition::base_cost)
        .def_ro("base_cps", &BuildingDefinition::base_cps);

    nb::class_<UpgradeDefinition>(m, "UpgradeDefinition")
        .def_ro("price", &UpgradeDefinition::price)
        .def_ro(
            "required_building",
            &UpgradeDefinition::requiredBuilding)
        .def_ro(
            "required_building_count",
            &UpgradeDefinition::requiredBuildingCount);

    nb::class_<Observation>(m, "Observation")
        .def_ro("current_simulation_time", &Observation::current_simulation_time)
        .def_ro("current_cookies", &Observation::current_cookies)
        .def_ro("all_time_cookies", &Observation::all_time_cookies)
        .def_ro("handmade_cookies", &Observation::handmade_cookies)
        .def_ro("total_cps", &Observation::total_cps)
        .def_ro("buildings_owned", &Observation::buildings_owned)
        .def_ro("can_buy_1", &Observation::can_buy_1)
        .def_ro("can_buy_10", &Observation::can_buy_10)
        .def_ro("can_buy_100", &Observation::can_buy_100)
        .def_ro("active_golden_cookie_buffs", &Observation::activeGoldenCookieBuffs)
        .def_ro("active_golden_cookie_buff_seconds_remaining", &Observation::activeGoldenCookieBuffSecondsRemaining)
        .def_ro("upgrades_owned", &Observation::upgrades_owned)
        .def_ro("upgrades_unlocked", &Observation::upgrades_unlocked)
        .def_ro("can_buy_upgrades", &Observation::can_buy_upgrades);

    nb::class_<StepResult>(m, "StepResult")
        .def_ro("obs", &StepResult::obs)
        .def_ro("reward", &StepResult::reward)
        .def_ro("terminated", &StepResult::terminated)
        .def_ro("truncated", &StepResult::truncated)
        .def_ro("done", &StepResult::done);

    nb::class_<Env>(m, "Env")
        .def(nb::init<>())
        .def(
            "reset",
            [](Env &env, const std::optional<unsigned int> seed)
            {
                return env.reset(seed);
            },
            "seed"_a = nb::none())
        .def(
            "step",
            [](Env &env, const int action_index)
            {
                return env.step(
                    actionFromDiscreteIndex(action_index));
            },
            "action_index"_a)
        .def(
            "step_action",
            &Env::step,
            "action"_a)
        .def("get_observation", &Env::get_observation)
        .def("is_terminal", &Env::is_terminal);

    m.def("action_from_discrete_index", &actionFromDiscreteIndex, "action_index"_a);

    m.attr("DISCRETE_ACTION_COUNT") = discreteActionCount;
    m.attr("BUILDING_COUNT") = +BuildingType::BUILDING_COUNT;
    m.attr("UPGRADE_COUNT") = +UpgradeType::UPGRADE_COUNT;
    m.attr("UPGRADE_ACTION_OFFSET") = upgradeActionOffset;

    m.attr("EPISODE_LENGTH") = Config::episode_length;
    m.attr("TARGET_COOKIES") = Config::target_cookies;
    m.attr("BUYING_TIME_COST") = Config::buying_time_cost;
    m.attr("CLICKING_FREQUENCY") = Config::clicking_frequency;
    m.attr("PROGRESS_SHAPING_BETA") = Config::progress_shaping_beta;
    m.attr("REWARD_MODE") = nb::cast(Config::reward_mode);

    m.attr("BUILDING_DEFINITIONS") = nb::cast(buildingsDefinitions);
    m.attr("UPGRADE_DEFINITIONS") = nb::cast(upgradeDefinitions);
}
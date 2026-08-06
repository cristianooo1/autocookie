#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>

#include "env.hpp"
#include "config.hpp"

NB_MODULE(autocookie, m)
{

    nanobind::enum_<BuildingType>(m, "BuildingType")
        .value("CURSOR", BuildingType::CURSOR)
        .value("GRANDMA", BuildingType::GRANDMA)
        .value("FARM", BuildingType::FARM);

    nanobind::enum_<ActionType>(m, "ActionType")
        .value("ClickCookie", ActionType::ClickCookie)
        .value("BuyBuilding", ActionType::BuyBuilding)
        .value("Wait", ActionType::Wait);

    nanobind::class_<Action>(m, "Action", nanobind::dynamic_attr())
        .def(nanobind::init<>())
        .def_rw("type", &Action::type)
        .def_rw("buildingIndex", &Action::buildingIndex);

    nanobind::class_<Config::BuildingDefinition>(m, "BuildingDefinition")
        .def(nanobind::init<>())
        .def_ro("base_cost", &Config::BuildingDefinition::base_cost)
        .def_ro("base_production", &Config::BuildingDefinition::base_production)
        .def_ro("cost_multiplier", &Config::BuildingDefinition::cost_multiplier);

    nanobind::class_<Observation>(m, "Observation", nanobind::dynamic_attr())
        .def(nanobind::init<>())
        .def_rw("current_cookies", &Observation::current_cookies)
        .def_rw("all_time_cookies", &Observation::all_time_cookies)
        .def_rw("buildings_owned", &Observation::buildings_owned)
        .def_rw("cps", &Observation::cps);

    nanobind::class_<StepResult>(m, "StepResult", nanobind::dynamic_attr())
        .def(nanobind::init<>())
        .def_rw("obs", &StepResult::obs)
        .def_rw("reward", &StepResult::reward)
        .def_rw("done", &StepResult::done);

    nanobind::class_<Env>(m, "Env")
        .def(nanobind::init<>())
        .def("step", &Env::step)
        .def("reset", &Env::reset);
}
#pragma once

#include <array>

#include "config.hpp"

enum class EventType
{
    GOLDEN_COOKIE = 0,
    WRINKLE,

    EVENT_COUNT,
};

enum class WakeUpDecisionEventType
{
    BuildingAvailable = 0,
    UpgradeAvailable,
    GoldenCookieSpawned,
    GoldenCookieBuffExpiration,
    WrinkleSpawned,
    BuffExpiration,
    EpisodeBoundary,

    EVENT_COUNT,
};

struct NextScheduledEvent
{
    WakeUpDecisionEventType event_type{};
    double absolute_timestamp{};
};

enum class GoldenCookieBuff
{
    LUCKY = 0,
    FRENZY,
    BUILDING_SPECIAL,
    DRAGON_HARVEST,
    DRAGON_FLIGHT,
    CLICK_FRENZY,
    EVERYTHING_MUST_GO,
    BLAB,

    GOLDEN_COOKIE_BUFF_COUNT,
};

struct ActiveGoldenCookieBuff
{
    GoldenCookieBuff buff_type{};
    double starts_at{0.0};
    double expires_at{Config::episode_length};
};

struct GoldenCookieBuffDefinition
{
    double weight{};
    double duration{};
};

constexpr std::array<GoldenCookieBuffDefinition, +GoldenCookieBuff::GOLDEN_COOKIE_BUFF_COUNT> GoldenCookieBuff_Definitions{
    GoldenCookieBuffDefinition{
        // LUCKY = 0
        .weight = 422000.0,
        .duration = 0.0,
    },
    GoldenCookieBuffDefinition{
        // FRENZY = 1
        .weight = 422000.0,
        .duration = 77.0,
    },
    GoldenCookieBuffDefinition{
        // BUILDING_SPECIAL = 2
        // .weight = 100000.0,
        .weight = 0.0,
        .duration = 30.0,
    },
    GoldenCookieBuffDefinition{
        // DRAGON_HARVEST = 3
        // .weight = 71400.0,
        .weight = 0.0,
        .duration = 60.0,
    },
    GoldenCookieBuffDefinition{
        // DRAGON_FLIGHT = 4
        // .weight = 71400.0,
        .weight = 0.0,
        .duration = 10.0,
    },
    GoldenCookieBuffDefinition{
        // CLICK_FRENZY = 5
        // .weight = 39700.0,
        .weight = 0.0,
        .duration = 13.0,
    },
    GoldenCookieBuffDefinition{
        // EVERYTHING_MUST_GO = 6
        // .weight = 19400.0,
        .weight = 0.0,
        .duration = 8.0,
    },
    GoldenCookieBuffDefinition{
        // BLAB = 7
        // .weight = 39.0,
        .weight = 0.0,
        .duration = 3.0,
    }};
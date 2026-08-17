#pragma once

#include <random> // for std::mt19937 and std::random_device
#include <deque>

#include "gameState.hpp"
#include "config.hpp"
#include "types.hpp"

#include "eventTypes.hpp"

class EventSystem
{
public:
    EventSystem();

    void generateEpisodeSeed();
    void setEpisodeSeed(unsigned int seed);
    unsigned int getEpisodeSeed();

    void generateNextGoldenCookieSpawn(GameState &state);
    double getNextGoldenCookieSpawn();

    GoldenCookieBuff rollGoldenCookieBuff();

    void processGoldenCookieBuff(GameState &state, const double rate);
    void removeGoldenCookieBuff(GameState &state);

private:
    std::mt19937 mt_;
    unsigned int current_seed_{};
    double next_golden_cookie_spawn_{Config::episode_length};

    // DEBUGGING AND TESTS AND INJECTIBLE EVENT SCHEDULES/OUTCOMES
    friend struct EnvTestAccess;

    struct ScriptedGoldenCookie
    {
        double absolute_timestamp{};
        GoldenCookieBuff buff{};
    };

    bool scripted_golden_cookies_enabled_{false};

    std::deque<ScriptedGoldenCookie> scripted_golden_cookies_{};
};
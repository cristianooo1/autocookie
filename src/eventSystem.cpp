#include <algorithm>

#include "eventSystem.hpp"

EventSystem::EventSystem()
{
}

void EventSystem::setEpisodeSeed(unsigned int seed)
{
    this->current_seed_ = seed;
    mt_.seed(seed);
}

void EventSystem::generateEpisodeSeed()
{

    this->setEpisodeSeed(std::random_device{}());
}

unsigned int EventSystem::getEpisodeSeed()
{
    return this->current_seed_;
}

void EventSystem::generateNextGoldenCookieSpawn(GameState &state)
{
    /*
   think pre-generated schedule vs
   incremental scheduling = know seed but generate event as simulation goes
   e.g. golden cookie spawning affected by upgrades

    THE GOLDEN COOKIE HAS A SPECIFIC CALCULATION, NOW USE BASIC ONE FOR EVERYTHING
    STATE is for calculting the specific probability of spawning and buff based on current upgrades
   */

    std::uniform_real_distribution<double> event_timestamp{state.current_simulation_time + 10.0, state.current_simulation_time + 50.0};

    double timestamp = event_timestamp(this->mt_);

    this->next_golden_cookie_spawn_ = timestamp;
}

double EventSystem::getNextGoldenCookieSpawn()
{
    return this->next_golden_cookie_spawn_;
}

GoldenCookieBuff EventSystem::rollGoldenCookieBuff()
{

    double total_weight = 0.0;

    for (const GoldenCookieBuffDefinition &buff : GoldenCookieBuff_Definitions)
    {
        total_weight += buff.weight;
    }

    std::uniform_real_distribution<double> weight_distribution{0.0, total_weight};

    // value between 0.0 and 1.145.939
    double random_buff_roll = weight_distribution(this->mt_);

    for (int i = 0; i < GoldenCookieBuff_Definitions.size(); ++i)
    {

        if (random_buff_roll < GoldenCookieBuff_Definitions[i].weight)
        {
            return static_cast<GoldenCookieBuff>(i);
        }

        /*
        LUCKY: 0 - 422.000
        FRENZY: 422.000 - 844.000
        BUILDING_SPECIAL: 844.000 - 944.000
        DRAGON_HARVEST: 944.000 - 1.015.400
        DRAGON_FLIGHT: 1.015.400 - 1.086.800
        CLICK_FRENZY: 1.086.800 - 1.126.500
        EVERYTHING_MUST_GO: 1.126.500 - 1.145.900
        BLAB: 1.145.900 - 1.145.939

        */
        random_buff_roll -= GoldenCookieBuff_Definitions[i].weight;
    }
    return static_cast<GoldenCookieBuff>(GoldenCookieBuff_Definitions.size() - 1);
}

void EventSystem::processGoldenCookieBuff(GameState &state, const double rate)
{

    // assume event == WakeUpDecisionEventType::GoldenCookieSpawned @!!!!!!
    // assume we reached the timestamp from next_golden_cookie_spawn_ !!!!!!

    // roll the buff
    GoldenCookieBuff buff = this->rollGoldenCookieBuff();

    if (buff == GoldenCookieBuff::LUCKY)
    {
        double cookies_banked_15 = state.current_cookies * 15 / 100 + 13;
        double cps_15_mins = rate * 900 + 13;
        double lucky_bonus = std::min(cookies_banked_15, cps_15_mins);

        state.current_cookies += lucky_bonus;
        state.alltime_cookies += lucky_bonus;
    }
    else
    {
        auto already_existing_buff = std::find_if(
            state.activeGoldenCookieBuffs.begin(),
            state.activeGoldenCookieBuffs.end(),
            [buff](const ActiveGoldenCookieBuff &active_buff)
            {
                return active_buff.buff_type == buff;
            });

        if (already_existing_buff != state.activeGoldenCookieBuffs.end())
        {
            already_existing_buff->expires_at += GoldenCookieBuff_Definitions[+buff].duration;
        }
        else
        {
            state.activeGoldenCookieBuffs.push_back(ActiveGoldenCookieBuff{
                .buff_type = buff,
                .starts_at = state.current_simulation_time,
                .expires_at = state.current_simulation_time + GoldenCookieBuff_Definitions[+buff].duration});
        }
    }

    // sort buffs by expiration time

    std::sort(state.activeGoldenCookieBuffs.begin(), state.activeGoldenCookieBuffs.end(),
              [](const ActiveGoldenCookieBuff &a, const ActiveGoldenCookieBuff &b)
              {
                  return a.expires_at < b.expires_at;
              });

    // schedule next golden cookie spawn
    this->generateNextGoldenCookieSpawn(state);
}

void EventSystem::removeGoldenCookieBuff(GameState &state)
{

    // assume event == WakeUpDecisionEventType::GoldenCookieBuffExpiration @!!!!!!
    //  assume we reached the timestamp from when a buff expires !!!!!!

    constexpr double time_epsilon = 1e-9;

    auto erased = std::erase_if(state.activeGoldenCookieBuffs, [&](const ActiveGoldenCookieBuff &buff)
                                { return buff.expires_at <= state.current_simulation_time + time_epsilon; });
}

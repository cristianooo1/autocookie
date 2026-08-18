#include <algorithm>

#include "eventSystem.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <cstdint>

EventSystem::EventSystem()
{
}

void EventSystem::setEpisodeSeed(unsigned int seed)
{

    scripted_golden_cookies_enabled_ = false;
    scripted_golden_cookies_.clear();

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

    if (scripted_golden_cookies_enabled_)
    {
        if (scripted_golden_cookies_.empty())
        {
            next_golden_cookie_spawn_ =
                Config::episode_length;
        }
        else
        {
            next_golden_cookie_spawn_ =
                scripted_golden_cookies_
                    .front()
                    .absolute_timestamp;
        }

#ifndef NDEBUG
        assert(std::isfinite(state.current_simulation_time));
        assert(std::isfinite(next_golden_cookie_spawn_));
        assert(
            state.current_simulation_time <
            Config::episode_length);
        assert(
            next_golden_cookie_spawn_ >
            state.current_simulation_time);
#endif

        return;
    }

    constexpr double checks_per_second = Config::golden_cookie_checks_per_second;

    const double min_time = Config::golden_cookie_min_time;

    const double max_time = Config::golden_cookie_max_time;

    const std::uint64_t min_tick = static_cast<std::uint64_t>(std::ceil(min_time * checks_per_second));

    const std::uint64_t max_tick = static_cast<std::uint64_t>(std::ceil(max_time * checks_per_second));

    std::uniform_real_distribution<double> random_roll{0.0, 1.0};

    std::uint64_t spawn_tick = max_tick;

    for (std::uint64_t tick = min_tick + 1; tick < max_tick; ++tick)
    {
        double progress =
            static_cast<double>(tick - min_tick) /
            static_cast<double>(max_tick - min_tick);

        double progress_squared = progress * progress;

        double spawn_probability = progress_squared *
                                   progress_squared *
                                   progress;

        if (random_roll(mt_) < spawn_probability)
        {
            spawn_tick = tick;
            break;
        }
    }

    next_golden_cookie_spawn_ = state.current_simulation_time + static_cast<double>(spawn_tick) / checks_per_second;

#ifndef NDEBUG
    assert(std::isfinite(state.current_simulation_time));
    assert(std::isfinite(next_golden_cookie_spawn_));

    assert(
        next_golden_cookie_spawn_ >
        state.current_simulation_time);

    assert(
        next_golden_cookie_spawn_ >=
        state.current_simulation_time +
            Config::golden_cookie_min_time);

    assert(
        next_golden_cookie_spawn_ <=
        state.current_simulation_time +
            Config::golden_cookie_max_time);
#endif
}

double EventSystem::getNextGoldenCookieSpawn()
{
    return this->next_golden_cookie_spawn_;
}

GoldenCookieBuff EventSystem::rollGoldenCookieBuff()
{
    if (scripted_golden_cookies_enabled_)
    {
        if (scripted_golden_cookies_.empty())
        {
            throw std::logic_error(
                "SCRIPTED GOLDEN COOKIE QUEUE IS EMPTY!!!!!!!!!!!!!!!!!!!!!!!!");
        }

        GoldenCookieBuff buff =
            scripted_golden_cookies_.front().buff;

        scripted_golden_cookies_.pop_front();

        return buff;
    }

    double total_weight = 0.0;

    for (const GoldenCookieBuffDefinition &buff : GoldenCookieBuff_Definitions)
    {
#ifndef NDEBUG
        assert(std::isfinite(buff.weight));
        assert(buff.weight >= 0.0);
#endif
        total_weight += buff.weight;
    }
#ifndef NDEBUG
    assert(std::isfinite(total_weight));
    assert(total_weight > 0.0);
#endif

    std::uniform_real_distribution<double> weight_distribution{0.0, total_weight};

    // value between 0.0 and sum of enable buff weights
    double random_buff_roll = weight_distribution(this->mt_);

    GoldenCookieBuff last_enabled_buff = GoldenCookieBuff::LUCKY;

    for (int i = 0; i < GoldenCookieBuff_Definitions.size(); ++i)
    {
        double weight = GoldenCookieBuff_Definitions[i].weight;

        if (weight <= 0.0)
        {
            continue;
        }

        last_enabled_buff = static_cast<GoldenCookieBuff>(i);

        if (random_buff_roll < weight)
        {
            return static_cast<GoldenCookieBuff>(i);
        }

        random_buff_roll -= weight;
    }
    return last_enabled_buff;
}

void EventSystem::processGoldenCookieBuff(GameState &state, const double rate)
{

// assume event == WakeUpDecisionEventType::GoldenCookieSpawned @!!!!!!
// assume we reached the timestamp from next_golden_cookie_spawn_ !!!!!!
#ifndef NDEBUG
    assert(std::isfinite(state.current_simulation_time));
    assert(std::isfinite(state.current_cookies));
    assert(std::isfinite(state.alltime_cookies));
    assert(std::isfinite(rate));
    assert(rate >= 0.0);
#endif

    state.last_golden_cookie_timestamp = state.current_simulation_time;
    state.has_seen_golden_cookie = true;

    // roll the buff
    GoldenCookieBuff buff = this->rollGoldenCookieBuff();

#ifndef NDEBUG
    const int buff_index = static_cast<int>(buff);

    assert(buff_index >= 0);
    assert(
        buff_index <
        +GoldenCookieBuff::GOLDEN_COOKIE_BUFF_COUNT);

    assert(
        GoldenCookieBuff_Definitions[static_cast<std::size_t>(buff_index)]
            .weight > 0.0);
#endif

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

#ifndef NDEBUG
    assert(std::is_sorted(
        state.activeGoldenCookieBuffs.begin(),
        state.activeGoldenCookieBuffs.end(),
        [](const ActiveGoldenCookieBuff &a,
           const ActiveGoldenCookieBuff &b)
        {
            return a.expires_at < b.expires_at;
        }));

    for (std::size_t i = 0;
         i < state.activeGoldenCookieBuffs.size();
         ++i)
    {
        const ActiveGoldenCookieBuff &active_buff =
            state.activeGoldenCookieBuffs[i];

        assert(std::isfinite(active_buff.starts_at));
        assert(std::isfinite(active_buff.expires_at));
        assert(
            active_buff.expires_at >
            state.current_simulation_time);

        for (std::size_t j = i + 1;
             j < state.activeGoldenCookieBuffs.size();
             ++j)
        {
            assert(
                active_buff.buff_type !=
                state.activeGoldenCookieBuffs[j].buff_type);
        }
    }
#endif

    // schedule next golden cookie spawn
    this->generateNextGoldenCookieSpawn(state);
}

void EventSystem::removeGoldenCookieBuff(GameState &state)
{

// assume event == WakeUpDecisionEventType::GoldenCookieBuffExpiration @!!!!!!
//  assume we reached the timestamp from when a buff expires !!!!!!
#ifndef NDEBUG
    assert(std::isfinite(state.current_simulation_time));

    assert(std::any_of(
        state.activeGoldenCookieBuffs.begin(),
        state.activeGoldenCookieBuffs.end(),
        [&](const ActiveGoldenCookieBuff &buff)
        {
            return buff.expires_at <=
                   state.current_simulation_time;
        }));
#endif

    auto erased = std::erase_if(state.activeGoldenCookieBuffs, [&](const ActiveGoldenCookieBuff &buff)
                                { return buff.expires_at <= state.current_simulation_time; });

#ifndef NDEBUG
    assert(std::none_of(
        state.activeGoldenCookieBuffs.begin(),
        state.activeGoldenCookieBuffs.end(),
        [&](const ActiveGoldenCookieBuff &buff)
        {
            return buff.expires_at <=
                   state.current_simulation_time;
        }));
#endif
}

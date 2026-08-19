from pathlib import Path
import sys

sys.path.append(str(Path(__file__).resolve().parent / "build"))

import autocookie
import gymnasium as gym
import numpy as np
from gymnasium import spaces


class CookieEnv(gym.Env):
    """
    Gymnasium wrapper for the C++ engine
    """

    # LUCKY is instantaneous so NEVER active
    # FRENZY and CLICK_FRENZY are the only buffs implemented for the 1mil cookie scenario
    active_buffs = (
        autocookie.GoldenCookieBuff.FRENZY,
        autocookie.GoldenCookieBuff.CLICK_FRENZY,
    )
    buff_indices = {buff: i for i, buff in enumerate(active_buffs)}

    def __init__(self):

        super().__init__()

        self.engine = autocookie.Env()
        self.action_space = spaces.Discrete(autocookie.DISCRETE_ACTION_COUNT)

        scalar = lambda high=np.inf: spaces.Box(0.0, high, (1,), np.float32)
        self.observation_space = spaces.Dict(
            {
                "current_simulation_time": scalar(autocookie.EPISODE_LENGTH),
                "current_cookies": scalar(),
                "all_time_cookies": scalar(),
                "handmade_cookies": scalar(),
                "total_cps": scalar(),
                "has_seen_golden_cookie": spaces.MultiBinary(1),
                "seconds_since_last_golden_cookie": scalar(autocookie.EPISODE_LENGTH),
                "buildings_owned": spaces.Box(0, np.iinfo(np.int32).max, (autocookie.BUILDING_COUNT,), np.int32),
                "upgrades_owned": spaces.MultiBinary(autocookie.UPGRADE_COUNT),
                "upgrades_unlocked": spaces.MultiBinary(autocookie.UPGRADE_COUNT),
                "active_golden_cookie_buffs": spaces.MultiBinary(len(self.active_buffs)),
                "active_golden_cookie_buff_seconds_remaining": spaces.Box(0.0, np.inf, (len(self.active_buffs),), np.float32),
            }
        )

        self._episode_seed = None
        self._action_mask = None

    def _convert_observation(self, obs):

        active_buffs = np.zeros(len(self.active_buffs), dtype=np.int8)
        buff_seconds = np.zeros(len(self.active_buffs), dtype=np.float32)

        for buff, seconds in zip(
            obs.active_golden_cookie_buffs,
            obs.active_golden_cookie_buff_seconds_remaining,
        ):
            index = self.buff_indices.get(buff)
            if index is not None:
                active_buffs[index] = 1
                buff_seconds[index] = seconds

        self._action_mask = np.asarray(obs.valid_action_mask, dtype=np.bool_)
        scalar = lambda value: np.array([value], dtype=np.float32)

        return {
            "current_simulation_time": scalar(obs.current_simulation_time),
            "current_cookies": scalar(obs.current_cookies),
            "all_time_cookies": scalar(obs.all_time_cookies),
            "handmade_cookies": scalar(obs.handmade_cookies),
            "total_cps": scalar(obs.total_cps),
            "has_seen_golden_cookie": np.array([obs.has_seen_golden_cookie], dtype=np.int8),
            "seconds_since_last_golden_cookie": scalar(obs.seconds_since_last_golden_cookie),
            "buildings_owned": np.asarray(obs.buildings_owned, dtype=np.int32),
            "upgrades_owned": np.asarray(obs.upgrades_owned, dtype=np.int8),
            "upgrades_unlocked": np.asarray(obs.upgrades_unlocked, dtype=np.int8),
            "active_golden_cookie_buffs": active_buffs,
            "active_golden_cookie_buff_seconds_remaining": buff_seconds,
        }

    def reset(self, *, seed=None, options=None):

        super().reset(seed=seed)

        # explicit seed = exact C++ seed
        # seed=None -> Gymnasium's RNG generates the next reproducible episode seed
        episode_seed = int(seed) if seed is not None else int(self.np_random.integers(0, 1 << 32, dtype=np.uint64))
        if not 0 <= episode_seed < 1 << 32:
            raise ValueError("seed must be in [0, 2**32)")

        self._episode_seed = episode_seed
        observation = self._convert_observation(self.engine.reset(episode_seed))
        return observation, {"episode_seed": episode_seed}

    def step(self, action):

        if not self.action_space.contains(action):
            raise ValueError(f"action must be in [0, {autocookie.DISCRETE_ACTION_COUNT})")

        result = self.engine.step(int(action))
        observation = self._convert_observation(result.obs)
        terminated = bool(result.terminated)
        truncated = bool(result.truncated)
        reached_target = bool(result.reached_target)
        reached_horizon = bool(result.reached_horizon)

        info = {
            "episode_seed": self._episode_seed,
            "reached_target": reached_target,
            "reached_horizon": reached_horizon,
        }
        if terminated or truncated:
            info["is_success"] = reached_target

        return observation, float(result.reward), terminated, truncated, info

    def action_masks(self):

        if self._action_mask is None:
            raise RuntimeError("reset() must be called before action_masks()")
        return self._action_mask


class NormalizeCookieObservation(gym.ObservationWrapper):

    normalized_keys = (
        "current_simulation_time",
        "current_cookies",
        "all_time_cookies",
        "handmade_cookies",
        "total_cps",
        "seconds_since_last_golden_cookie",
        "buildings_owned",
        "active_golden_cookie_buff_seconds_remaining",
    )
    buff_scales = np.array([77.0, 13.0], dtype=np.float32)

    def __init__(self, env):

        super().__init__(env)
        if not isinstance(env.observation_space, spaces.Dict):
            raise TypeError("observation_space must be spaces.Dict")

        normalized_spaces = dict(env.observation_space.spaces)
        for key in self.normalized_keys:
            normalized_spaces[key] = spaces.Box(0.0, 1.0, normalized_spaces[key].shape, np.float32)
        self.observation_space = spaces.Dict(normalized_spaces)
        self.cookie_scale = np.log1p(float(autocookie.TARGET_COOKIES))
        self.building_scale = np.log1p(100.0)

    def observation(self, observation):

        normalized = dict(observation)
        normalized["current_simulation_time"] = np.clip(observation["current_simulation_time"] / autocookie.EPISODE_LENGTH, 0.0, 1.0).astype(
            np.float32
        )
        normalized["seconds_since_last_golden_cookie"] = np.clip(observation["seconds_since_last_golden_cookie"] / 900.0, 0.0, 1.0).astype(np.float32)

        for key in (
            "current_cookies",
            "all_time_cookies",
            "handmade_cookies",
            "total_cps",
        ):
            normalized[key] = np.clip(
                np.log1p(np.maximum(observation[key], 0.0)) / self.cookie_scale,
                0.0,
                1.0,
            ).astype(np.float32)

        normalized["buildings_owned"] = np.clip(
            np.log1p(np.maximum(observation["buildings_owned"], 0)) / self.building_scale,
            0.0,
            1.0,
        ).astype(np.float32)

        seconds = np.maximum(observation["active_golden_cookie_buff_seconds_remaining"], 0.0)
        normalized["active_golden_cookie_buff_seconds_remaining"] = (seconds / (seconds + self.buff_scales)).astype(np.float32)
        return normalized

    def action_masks(self):

        return self.env.get_wrapper_attr("action_masks")()

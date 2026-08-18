import sys
import os
from typing import Any, Optional

# PROJECT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
# BUILD_DIRECTORY = os.path.join(PROJECT_DIRECTORY, "build")
# if BUILD_DIRECTORY not in sys.path:
#     sys.path.append(BUILD_DIRECTORY)


sys.path.append(os.path.abspath("build"))

import autocookie

import numpy as np
import gymnasium as gym
from gymnasium import spaces
import pygame

class CookieEnv(gym.Env):
    metadata = {"render_modes": ["human", "ansi"], "render_fps": 5}

    building_names = ("Cursor", "Grandma", "Farm", "Mine", "Factory")
    buying_quantities = (1, 10, 100)
    upgrade_names = (
        "Reinforced index finger",
        "Carpal tunnel prevention cream",
        "Ambidextrous",
        "Thousand fingers",
        "Plastic mouse",
        "Forwards from grandma",
        "Steel-plated rolling pins",
        "Lubricated dentures",
        "Farmer grandmas",
        "Cheap hoes",
        "Fertilizer",
        "Cookie trees",
        "Sugar gas",
        "Megadrill",
    )
    buff_types = (
        autocookie.GoldenCookieBuff.LUCKY,
        autocookie.GoldenCookieBuff.FRENZY,
        autocookie.GoldenCookieBuff.BUILDING_SPECIAL,
        autocookie.GoldenCookieBuff.DRAGON_HARVEST,
        autocookie.GoldenCookieBuff.DRAGON_FLIGHT,
        autocookie.GoldenCookieBuff.CLICK_FRENZY,
        autocookie.GoldenCookieBuff.EVERYTHING_MUST_GO,
        autocookie.GoldenCookieBuff.BLAB,
    )
    buff_names = (
        "Lucky",
        "Frenzy",
        "Building special",
        "Dragon harvest",
        "Dragonflight",
        "Click frenzy",
        "Everything must go",
        "Blab",
    )
    buff_indices = {
        buff_type: index for index, buff_type in enumerate(buff_types)
    }

    def __init__(
        self,
        render_mode: Optional[str] = None,
        render_speed: float = 1.0,
    ):
        super().__init__()

        if render_mode is not None and render_mode not in self.metadata["render_modes"]:
            raise ValueError(f"Unsupported render mode: {render_mode}")
        if not np.isfinite(render_speed) or render_speed <= 0.0:
            raise ValueError("render_speed must be a positive finite number")

        self.engine = autocookie.Env()
        self.render_mode = render_mode
        self.render_speed = float(render_speed)
        self.render_fps = max(
            1,
            round(self.metadata["render_fps"] * self.render_speed),
        )

        self.action_space = spaces.Discrete(autocookie.DISCRETE_ACTION_COUNT)

        building_count = autocookie.BUILDING_COUNT
        upgrade_count = autocookie.UPGRADE_COUNT
        buff_count = len(self.buff_types)

        self.observation_space = spaces.Dict(
            {
                "current_simulation_time": spaces.Box(
                    low=0.0,
                    high=autocookie.EPISODE_LENGTH,
                    shape=(1,),
                    dtype=np.float32,
                ),
                "current_cookies": spaces.Box(
                    low=0.0, high=np.inf, shape=(1,), dtype=np.float32
                ),
                "all_time_cookies": spaces.Box(
                    low=0.0, high=np.inf, shape=(1,), dtype=np.float32
                ),
                "handmade_cookies": spaces.Box(
                    low=0.0, high=np.inf, shape=(1,), dtype=np.float32
                ),
                "total_cps": spaces.Box(
                    low=0.0, high=np.inf, shape=(1,), dtype=np.float32
                ),
                "buildings_owned": spaces.Box(
                    low=0,
                    high=np.iinfo(np.int32).max,
                    shape=(building_count,),
                    dtype=np.int32,
                ),
                "can_buy_1": spaces.MultiBinary(building_count),
                "can_buy_10": spaces.MultiBinary(building_count),
                "can_buy_100": spaces.MultiBinary(building_count),
                "upgrades_owned": spaces.MultiBinary(upgrade_count),
                "upgrades_unlocked": spaces.MultiBinary(upgrade_count),
                "can_buy_upgrades": spaces.MultiBinary(upgrade_count),
                "active_golden_cookie_buffs": spaces.MultiBinary(buff_count),
                "active_golden_cookie_buff_seconds_remaining": spaces.Box(
                    low=0.0,
                    high=np.inf,
                    shape=(buff_count,),
                    dtype=np.float32,
                ),
            }
        )

        # pygame for rendering setup
        self.window = None
        self.clock = None
        self.font = None
        self.action_history: list[str] = []
        self.max_history: int = 11
        self.last_buildings = np.zeros(building_count, dtype=np.int32)
        self.last_upgrades = np.zeros(upgrade_count, dtype=np.int8)
        self.last_observation: Optional[dict[str, np.ndarray]] = None
        self.purchase_effect: Optional[dict[str, Any]] = None

    def _convert_observation(self, obs_struct):
        active_buffs = np.zeros(len(self.buff_types), dtype=np.int8)
        active_buff_seconds_remaining = np.zeros(
            len(self.buff_types), dtype=np.float32
        )

        for buff_type, seconds_remaining in zip(
            obs_struct.active_golden_cookie_buffs,
            obs_struct.active_golden_cookie_buff_seconds_remaining,
        ):
            buff_index = self.buff_indices[buff_type]
            active_buffs[buff_index] = 1
            active_buff_seconds_remaining[buff_index] = seconds_remaining

        return {
            "current_simulation_time": np.array(
                [obs_struct.current_simulation_time], dtype=np.float32
            ),
            "current_cookies": np.array(
                [obs_struct.current_cookies], dtype=np.float32
            ),
            "all_time_cookies": np.array(
                [obs_struct.all_time_cookies], dtype=np.float32
            ),
            "handmade_cookies": np.array(
                [obs_struct.handmade_cookies], dtype=np.float32
            ),
            "total_cps": np.array([obs_struct.total_cps], dtype=np.float32),
            "buildings_owned": np.asarray(
                obs_struct.buildings_owned, dtype=np.int32
            ).reshape(-1),
            "can_buy_1": np.asarray(obs_struct.can_buy_1, dtype=np.int8),
            "can_buy_10": np.asarray(obs_struct.can_buy_10, dtype=np.int8),
            "can_buy_100": np.asarray(obs_struct.can_buy_100, dtype=np.int8),
            "upgrades_owned": np.asarray(
                obs_struct.upgrades_owned, dtype=np.int8
            ),
            "upgrades_unlocked": np.asarray(
                obs_struct.upgrades_unlocked, dtype=np.int8
            ),
            "can_buy_upgrades": np.asarray(
                obs_struct.can_buy_upgrades, dtype=np.int8
            ),
            "active_golden_cookie_buffs": active_buffs,
            "active_golden_cookie_buff_seconds_remaining": (
                active_buff_seconds_remaining
            ),
        }

    def reset(
        self,
        *,
        seed: Optional[int] = None,
        options: Optional[dict[str, Any]] = None,
    ):
        super().reset(seed=seed, options=options)

        if seed is None:
            obs_struct = self.engine.reset()
        else:
            maximum_seed = np.iinfo(np.uint32).max
            if seed < 0 or seed > maximum_seed:
                raise ValueError(
                    f"seed must be between 0 and {maximum_seed}, inclusive"
                )
            obs_struct = self.engine.reset(int(seed))

        observation = self._convert_observation(obs_struct)
        info = {}

        self.action_history.clear()
        self.last_buildings = observation["buildings_owned"].copy()
        self.last_upgrades = observation["upgrades_owned"].copy()
        self.last_observation = observation
        self.purchase_effect = None

        return observation, info

    def _action_name(self, action: int) -> str:
        if action == 0:
            return "Advance"

        if action < autocookie.UPGRADE_ACTION_OFFSET:
            purchase_index = action - 1
            building_index = purchase_index // len(self.buying_quantities)
            quantity_index = purchase_index % len(self.buying_quantities)
            building_name = self.building_names[building_index]
            quantity = self.buying_quantities[quantity_index]
            suffix = "" if quantity == 1 else "s"
            return f"Buy {quantity} {building_name}{suffix}"

        upgrade_index = action - autocookie.UPGRADE_ACTION_OFFSET
        return f"Buy {self.upgrade_names[upgrade_index]}"

    def action_masks(self) -> np.ndarray:
        """Return the currently valid actions for MaskablePPO."""
        observation = self.last_observation
        if observation is None:
            observation = self._convert_observation(
                self.engine.get_observation()
            )

        mask = np.zeros(
            autocookie.DISCRETE_ACTION_COUNT,
            dtype=np.bool_,
        )

        mask[0] = True

        for building_index in range(autocookie.BUILDING_COUNT):
            action_offset = 1 + building_index * len(
                self.buying_quantities
            )
            mask[action_offset] = bool(
                observation["can_buy_1"][building_index]
            )
            mask[action_offset + 1] = bool(
                observation["can_buy_10"][building_index]
            )
            mask[action_offset + 2] = bool(
                observation["can_buy_100"][building_index]
            )

        mask[
            autocookie.UPGRADE_ACTION_OFFSET:
        ] = observation["can_buy_upgrades"].astype(
            np.bool_,
            copy=False,
        )

        return mask

    def step(self, action):
        if not self.action_space.contains(action):
            raise ValueError(
                f"action must be in [0, "
                f"{autocookie.DISCRETE_ACTION_COUNT - 1}], got {action}"
            )
        action = int(action)

        step_result = self.engine.step(action)

        observation = self._convert_observation(step_result.obs)
        reward = float(step_result.reward)

        terminated = bool(step_result.terminated)
        truncated = bool(step_result.truncated)
        info = {}

        self.action_history.append(self._action_name(action))
        if len(self.action_history) > self.max_history:
            self.action_history.pop(0)

        new_buildings = observation["buildings_owned"]
        diff = new_buildings - self.last_buildings
        purchased_buildings = np.flatnonzero(diff > 0)

        if purchased_buildings.size > 0:
            building_index = int(purchased_buildings[0])
            quantity = int(diff[building_index])
            building_name = self.building_names[building_index]
            suffix = "" if quantity == 1 else "s"
            self.purchase_effect = {
                "text": f"+{quantity} {building_name}{suffix}!",
                "timer": 5,
                "y_offset": 0.0,
            }
        else:
            new_upgrades = observation["upgrades_owned"]
            purchased_upgrades = np.flatnonzero(
                new_upgrades > self.last_upgrades
            )
            if purchased_upgrades.size > 0:
                upgrade_index = int(purchased_upgrades[0])
                self.purchase_effect = {
                    "text": f"{self.upgrade_names[upgrade_index]}!",
                    "timer": 5,
                    "y_offset": 0.0,
                }

        self.last_buildings = new_buildings.copy()
        self.last_upgrades = observation["upgrades_owned"].copy()
        self.last_observation = observation

        return observation, reward, terminated, truncated, info

    def render(self):
        if self.last_observation is None:
            self.last_observation = self._convert_observation(
                self.engine.get_observation()
            )

        observation = self.last_observation
        time = float(observation["current_simulation_time"][0])
        cookies = float(observation["current_cookies"][0])
        all_time = float(observation["all_time_cookies"][0])
        handmade = float(observation["handmade_cookies"][0])
        cps = float(observation["total_cps"][0])
        buildings = observation["buildings_owned"]
        upgrades_owned = int(observation["upgrades_owned"].sum())
        active_buff_indices = np.flatnonzero(
            observation["active_golden_cookie_buffs"]
        )
        active_buff_text = (
            ", ".join(
                self.buff_names[int(buff_index)]
                for buff_index in active_buff_indices
            )
            if active_buff_indices.size > 0
            else "None"
        )

        ansi_text = (
            f"cookies: {cookies:.2f} | all-time: {all_time:.2f} | "
            f"CPS: {cps:.2f} | time: {time:.2f}"
        )

        if self.render_mode == "human":
            if self.window is None:
                pygame.init()
                pygame.display.init()
                self.window = pygame.display.set_mode((800, 600))
                pygame.display.set_caption("auto cookie rl")
                self.clock = pygame.time.Clock()
                self.font = pygame.font.SysFont("liberationmono", 25, bold=True)

            pygame.event.pump()
            self.window.fill("#34C0EB")  # BACKGROUND COLOR

            if self.font is not None:
                # LEFT PANEL STATS
                texts = [
                    f"time: {time:.1f} / {autocookie.EPISODE_LENGTH:.0f}",
                    f"curent coooookies: {cookies:.1f}",
                    f"ALLTIME coooookies: {all_time:.1f}",
                    f"HANDMADE coooookies: {handmade:.1f}",
                    f"CPS: {cps:.1f}",
                    "$$$$$$$ Buildings",
                    f"CURSORS: {buildings[0]}",
                    f"GRANDMAS: {buildings[1]}",
                    f"FARMS: {buildings[2]}",
                    f"MINES: {buildings[3]}",
                    f"FACTORIES: {buildings[4]}",
                    f"UPGRADES: {upgrades_owned}/{autocookie.UPGRADE_COUNT}",
                    f"BUFFS: {active_buff_text}",
                ]

                y_offset = 20
                for text in texts:
                    color = pygame.Color("#C80000")  # STATS ON THE LEFT
                    text_surface = self.font.render(text, True, color)
                    self.window.blit(text_surface, (20, y_offset))
                    y_offset += 35

                pygame.draw.line(
                    self.window,
                    pygame.Color("#060000"),
                    (390, 20),
                    (390, 500),
                    2,
                )  # DIVIDER LINE

                # RIGHT PANEL = ACTION HISTORY
                history_title = self.font.render(
                    "LAST ACTIONS", True, pygame.Color("#C8C8C8")
                )
                self.window.blit(history_title, (420, 20))

                hist_y_offset = 55
                for hist_text in self.action_history:
                    hist_color = (
                        pygame.Color("#FFD700")
                        if "Buy" in hist_text
                        else pygame.Color("#FFFFFF")
                    )
                    hist_surface = self.font.render(
                        str(hist_text), True, hist_color
                    )
                    self.window.blit(hist_surface, (420, hist_y_offset))
                    hist_y_offset += 30

                # SPECIAL EFFECTS
                if (
                    self.purchase_effect is not None
                    and self.purchase_effect["timer"] > 0
                ):
                    effect_color = pygame.Color("#32FF32")
                    effect_text = str(self.purchase_effect["text"])
                    effect_surface = self.font.render(
                        effect_text, True, effect_color
                    )

                    current_y = 150 - self.purchase_effect["y_offset"]
                    self.window.blit(effect_surface, (620, current_y))

                    self.purchase_effect["y_offset"] += 4.0
                    self.purchase_effect["timer"] -= 1

            # UPDATE DISPLAY AFTER DRAWING EVERYTHING
            pygame.display.flip()
            if self.clock is not None:
                self.clock.tick_busy_loop(self.render_fps)

            return None

        if self.render_mode == "ansi":
            return ansi_text

        return None

    def close(self):
        if self.window is not None:
            pygame.display.quit()
            pygame.quit()
            self.window = None
            self.clock = None
            self.font = None


class NormalizeCookieObservation(gym.ObservationWrapper):
    """Normalize agent inputs while preserving CookieEnv's raw state."""

    _normalized_keys = (
        "current_simulation_time",
        "current_cookies",
        "all_time_cookies",
        "handmade_cookies",
        "total_cps",
        "buildings_owned",
        "active_golden_cookie_buff_seconds_remaining",
    )


    _handmade_cookie_scale = 1_000.0
    _building_count_scale = np.log1p(100.0)

    _buff_duration_scales = np.asarray(
        [1.0, 77.0, 30.0, 60.0, 10.0, 13.0, 8.0, 3.0],
        dtype=np.float32,
    )

    def __init__(self, env: gym.Env):
        super().__init__(env)

        if not isinstance(env.observation_space, spaces.Dict):
            raise TypeError(
                "NormalizeCookieObservation requires a Dict observation space"
            )

        normalized_spaces = dict(env.observation_space.spaces)
        for key in self._normalized_keys:
            original_space = normalized_spaces[key]
            normalized_spaces[key] = spaces.Box(
                low=0.0,
                high=1.0,
                shape=original_space.shape,
                dtype=np.float32,
            )

        self.observation_space = spaces.Dict(normalized_spaces)
        self.last_raw_observation: Optional[
            dict[str, np.ndarray]
        ] = None
        self._cookie_log_scale = np.log1p(
            float(autocookie.TARGET_COOKIES)
        )

    def _normalize_logarithmically(
        self,
        values: np.ndarray,
    ) -> np.ndarray:
        normalized = (
            np.log1p(np.maximum(values, 0.0))
            / self._cookie_log_scale
        )
        return np.clip(normalized, 0.0, 1.0).astype(
            np.float32,
            copy=False,
        )

    def observation(
        self,
        observation: dict[str, np.ndarray],
    ) -> dict[str, np.ndarray]:

        self.last_raw_observation = observation
        normalized = observation.copy()

        normalized["current_simulation_time"] = np.clip(
            observation["current_simulation_time"]
            / float(autocookie.EPISODE_LENGTH),
            0.0,
            1.0,
        ).astype(np.float32, copy=False)

        for key in ("current_cookies", "all_time_cookies", "total_cps"):
            normalized[key] = self._normalize_logarithmically(
                observation[key]
            )

        normalized["handmade_cookies"] = np.clip(
            observation["handmade_cookies"] / self._handmade_cookie_scale,
            0.0,
            1.0,
        ).astype(np.float32, copy=False)

        normalized["buildings_owned"] = np.clip(
            np.log1p(
                np.maximum(observation["buildings_owned"], 0)
            )
            / self._building_count_scale,
            0.0,
            1.0,
        ).astype(np.float32, copy=False)

        buff_seconds = np.maximum(
            observation[
                "active_golden_cookie_buff_seconds_remaining"
            ],
            0.0,
        ).astype(np.float32, copy=False)
        normalized[
            "active_golden_cookie_buff_seconds_remaining"
        ] = (
            buff_seconds
            / (buff_seconds + self._buff_duration_scales)
        ).astype(np.float32, copy=False)

        return normalized

    def action_masks(self) -> np.ndarray:
        """Forward action-mask queries through Gymnasium wrappers."""
        get_action_masks = self.env.get_wrapper_attr("action_masks")
        return np.asarray(get_action_masks(), dtype=np.bool_)
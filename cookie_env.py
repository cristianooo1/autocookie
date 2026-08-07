import sys
import os

sys.path.append(os.path.abspath("build"))
import autocookie

import numpy as np
import gymnasium as gym
from gymnasium import spaces
from typing import Optional, Any
import pygame

class CookieEnv(gym.Env):
    metadata = {"render_modes": ["human", "ansi"]}
    
    def __init__(self, render_mode: Optional[str] = None):
        super().__init__()
        
        self.engine = autocookie.Env()
        
        self.render_mode = render_mode
        
        # 5 discrete actions so far
        self.action_space = spaces.Discrete(5) 
        
        # 6 observations so far
        self.observation_space = spaces.Dict({
            "current_cookies": spaces.Box(low=0.0, high=np.inf, shape=(1,), dtype=np.float32),
            "all_time_cookies": spaces.Box(low=0.0, high=np.inf, shape=(1,), dtype=np.float32),
            "buildings_owned": spaces.Box(low=0, high=np.iinfo(np.int32).max, shape=(3,), dtype=np.int32),
            "cps": spaces.Box(low=0.0, high=np.inf, shape=(1,), dtype=np.float32),
        })
        
        # pygame for rendering setup
        self.window = None
        self.clock = None
        self.font = None
        self.action_history: list[str] = []
        self.max_history: int = 11
        self.last_buildings = np.zeros(3, dtype=np.int32)
        self.purchase_effect: Optional[dict[str, Any]] = None
    
    def _convert_observation(self, obs_struct):
        return {
            "current_cookies": np.array([obs_struct.current_cookies], dtype=np.float32),
            "all_time_cookies": np.array([obs_struct.all_time_cookies], dtype=np.float32),
            "buildings_owned": np.array(obs_struct.buildings_owned, dtype=np.int32).flatten(),
            "cps": np.array([obs_struct.cps], dtype=np.float32),
        }
        
    def reset(
        self,
        *,
        seed: Optional[int] = None,
        options: Optional[dict[str, Any]] = None,
    ):
        super().reset(seed=seed, options=options)
        
        obs_struct = self.engine.reset()
        observation = self._convert_observation(obs_struct)
        info = {}
        
        self.action_history.clear()
        self.last_buildings = np.zeros(3, dtype=np.int32)
        self.purchase_effect = None

        return observation, info
    
    def step(self, action):
        cpp_action = autocookie.Action()

        if action == 0:
            cpp_action.type = autocookie.ActionType.ClickCookie
        elif action == 1:
            cpp_action.type = autocookie.ActionType.Wait
        elif action == 2:
            cpp_action.type = autocookie.ActionType.BuyBuilding
            cpp_action.buildingIndex = autocookie.BuildingType.CURSOR
        elif action == 3:
            cpp_action.type = autocookie.ActionType.BuyBuilding
            cpp_action.buildingIndex = autocookie.BuildingType.GRANDMA
        elif action == 4:
            cpp_action.type = autocookie.ActionType.BuyBuilding
            cpp_action.buildingIndex = autocookie.BuildingType.FARM

        step_result = self.engine.step(cpp_action)

        observation = self._convert_observation(step_result.obs)
        reward = float(step_result.reward)
        
        # terminal when state.time == 100 CHANGE THIS IN THE CPP CODE!!!!!!!!!!!!!!!!!!!!!!!!!!
        terminated = bool(step_result.done)
        truncated = False  
        info = {}
        
        action_names = {0: "Click Cookie", 1: "Wait", 2: "Buy Cursor", 3: "Buy Grandma", 4: "Buy Farm"}
        action_str = action_names.get(int(action), "Unknown")
        self.action_history.append(action_str)
        if len(self.action_history) > self.max_history:
            self.action_history.pop(0)

        new_buildings = observation["buildings_owned"]
        diff = new_buildings - self.last_buildings
        
        if diff[0] > 0:
            self.purchase_effect = {"text": "+1 Cursor!", "timer": 5, "y_offset": 0.0}
        elif diff[1] > 0:
            self.purchase_effect = {"text": "+1 Grandma!", "timer": 5, "y_offset": 0.0}
        elif diff[2] > 0:
            self.purchase_effect = {"text": "+1 Farm!", "timer": 5, "y_offset": 0.0}
            
        self.last_buildings = new_buildings.copy()

        return observation, reward, terminated, truncated, info
    
    def render(self):
        state_tuple_cpp = self.engine.queryState()
        cookies, all_time, cps, time, cursors, grandmas, farms = state_tuple_cpp

        if self.render_mode == "human":
            if self.window is None:
                pygame.init()
                pygame.display.init()
                self.window = pygame.display.set_mode((800, 600))
                pygame.display.set_caption("auto cookie rl")
                self.clock = pygame.time.Clock()
                self.font = pygame.font.SysFont("liberationmono", 25, bold=True)

            pygame.event.pump()
            self.window.fill(("#34C0EB")) # BACKGROUND COLOR

            if self.font is not None:
                
                # LEFT PANEL STATS
                texts = [
                    f"time: {time:.1f} / 100",
                    f"curent coooookies: {cookies:.1f}",
                    f"ALLTIME coooookies: {all_time:.1f}",
                    f"CPS: {cps:.1f}",
                    "$$$$$$$ Buildings",
                    f"CURSORS: {cursors}",
                    f"GRANDMAS: {grandmas}",
                    f"FARMS: {farms}"
                ]

                y_offset = 20
                for text in texts:
                    color = pygame.Color("#C80000") # STATS ON THE LEFT
                    text_surface = self.font.render(text, True, color)
                    self.window.blit(text_surface, (20, y_offset))
                    y_offset += 35
                    
                pygame.draw.line(
                    self.window,
                    pygame.Color("#060000"), (390, 20), (390, 380), 2) # DIVIDER LINE 

                # RIGHT PANEL = ACTION HISTORY
                history_title = self.font.render("LAST ACTIONS", True, pygame.Color("#C8C8C8"))
                self.window.blit(history_title, (420, 20))
                
                hist_y_offset = 55
                for hist_text in self.action_history:
                    hist_color = pygame.Color("#FFD700") if "Buy" in hist_text else pygame.Color("#FFFFFF")
                    hist_surface = self.font.render(str(hist_text), True, hist_color) 
                    self.window.blit(hist_surface, (420, hist_y_offset))
                    hist_y_offset += 30

                # SPECIAL EFFECTS
                if self.purchase_effect is not None and self.purchase_effect["timer"] > 0:
                    effect_color = pygame.Color("#32FF32") 
                    effect_text = str(self.purchase_effect["text"]) 
                    effect_surface = self.font.render(effect_text, True, effect_color)
                    
                    current_y = 150 - self.purchase_effect["y_offset"]
                    self.window.blit(effect_surface, (620, current_y))
                    
                    self.purchase_effect["y_offset"] += 4.0 
                    self.purchase_effect["timer"] -= 1
                    
            # UPDATE DISPLAY AFTER DRAWING EVERYTHING
            pygame.display.flip()
            if self.clock is not None:
                self.clock.tick_busy_loop(5.0)

        else:
            print(f"cookies: {cookies:.2f} | CPS: {cps:.2f} | Time: {time}")

    def close(self):
        if self.window is not None:
            pygame.display.quit()
            pygame.quit()
            self.window = None
        
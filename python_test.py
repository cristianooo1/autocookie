import numpy as np
import gymnasium as gym
from gymnasium.envs.registration import register

import cookie_env

from typing import Optional, Any


def main():
    
    register(
    id="CookieClicker-v0",
    entry_point="cookie_env:CookieEnv",
)
    env = gym.make("CookieClicker-v0", render_mode="human")
    observation, info = env.reset()

    print("Initial Observation:", observation)

    terminated = False
    truncated = False
    total_reward = 0.0

    while not (terminated or truncated):
        action = env.action_space.sample()  
        observation, reward, terminated, truncated, info = env.step(action)
        total_reward += float(reward)
        env.unwrapped.render()
    env.close()

if __name__ == "__main__":
    main()

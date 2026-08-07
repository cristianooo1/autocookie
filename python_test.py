import os
# os.environ["TORCH_COMPILE_DISABLE"] = "1"
# os.environ["TRITON_DISABLE"] = "1"

import torch
import triton

import numpy as np
import gymnasium as gym
from gymnasium.envs.registration import register
from stable_baselines3 import A2C, PPO
from stable_baselines3.common.env_util import make_vec_env

import cookie_env

from typing import Optional, Any


def main():
    
    register(
    id="CookieClicker-v0",
    entry_point="cookie_env:CookieEnv",
    )
    
    # train_env = gym.make("CookieClicker-v0")
    train_env = make_vec_env("CookieClicker-v0", n_envs=8)
    
    model = PPO(
        "MultiInputPolicy",
        train_env,
        verbose=1,
        device="cpu",
        tensorboard_log="./cookie_logs/",
        seed=42,
        )
    print("STARTED TRAINING")
    model.learn(total_timesteps=5000, tb_log_name="PPO_5000steps_seed42") # 250_000 for real training
    print("FINISHED TRAINING")
    
    test_env = gym.make("CookieClicker-v0", render_mode="human")
    
    observation, info = test_env.reset()
    print("Initial Observation:", observation)

    terminated = False # is_terminal returns true after 100 time steps in the cpp code
    truncated = False
    total_reward = 0.0

    while not (terminated or truncated):

        # deterministic=true: select best actions based on what agent learned, without random exploration
        action, _state = model.predict(observation, deterministic=True)  
        
        observation, reward, terminated, truncated, info = test_env.step(action)
        
        total_reward += float(reward)
        
        test_env.unwrapped.render()
        
    print(f"Episode ended, total reward: {total_reward}!")    
    train_env.close()
    test_env.close()

if __name__ == "__main__":
    main()

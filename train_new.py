# os.environ["TORCH_COMPILE_DISABLE"] = "1"
# os.environ["TRITON_DISABLE"] = "1"

import torch
import triton

"""
ABOVE --^ imports are necessary, otherwise the script doesn't run
DONT DELETE THEM!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
"""


import csv
import json
from pathlib import Path

import numpy as np
from sb3_contrib import MaskablePPO
from sb3_contrib.common.maskable.callbacks import MaskableEvalCallback
from stable_baselines3.common.callbacks import BaseCallback, CheckpointCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.vec_env import SubprocVecEnv, DummyVecEnv
from gymnasium.spaces import Discrete

import wandb
from wandb.integration.sb3 import WandbCallback

from cookie_env_new import CookieEnv, NormalizeCookieObservation, autocookie

TOTAL_TIMESTEPS = 150_000
N_ENVS = 8

TRAINING_SEED = 42
EVALUATION_SEED = 10_000

EVALUATE_EVERY = 10_000
CHECKPOINT_EVERY = 25_000

LEARNING_RATE = 3e-4


# Dummy is faster than Subproc!!!!!!!!!!!!!!
VEC_ENV = DummyVecEnv
# VEC_ENV = SubprocVecEnv

BUILDINGS = ("Cursor", "Grandma", "Farm", "Mine", "Factory")
QUANTITIES = (1, 10, 100)
UPGRADES = (
    "Reinforced Index Finger",
    "Carpal Tunnel Prevention Cream",
    "Ambidextrous",
    "Thousand Fingers",
    "Plastic Mouse",
    "Forwards From Grandma",
    "Steel-Plated Rolling Pins",
    "Lubricated Dentures",
    "Farmer Grandmas",
    "Cheap Hoes",
    "Fertilizer",
    "Cookie Trees",
    "Sugar Gas",
    "Megadrill",
)


def action_name(action: int) -> str:
    if action == 0:
        return "Advance"
    action -= 1
    building_actions = len(BUILDINGS) * len(QUANTITIES)
    if action < building_actions:
        building, quantity = divmod(action, len(QUANTITIES))
        return f"Buy {BUILDINGS[building]} x{QUANTITIES[quantity]}"
    return f"Buy {UPGRADES[action - building_actions]}"


def compact_json(value) -> str:
    return json.dumps(value, default=lambda x: np.asarray(x).tolist(), separators=(",", ":"))


class EvaluationEnv(CookieEnv):
    def reset(self, *, seed=None, options=None):
        return super().reset(seed=EVALUATION_SEED, options=options)


class VerifyNormalizationCallback(BaseCallback):
    """
    check normalized observations after the first and final training steps
    """

    def _check(self, when: str) -> None:
        for key in NormalizeCookieObservation.normalized_keys:
            values = np.asarray(self.locals["new_obs"][key])
            if not np.all(np.isfinite(values) & (values >= 0) & (values <= 1)):
                raise RuntimeError(f"{key} is not normalized at the {when}")
        print(f"Normalization passed at the {when}.")

    def _on_step(self) -> bool:
        if self.n_calls == 1:
            self._check("first step")
        return True

    def _on_training_end(self) -> None:
        self._check("last step")


class TrainingCsvCallback(BaseCallback):
    def __init__(self, path: Path):
        super().__init__()
        self.path = path

    def _on_training_start(self) -> None:
        self.file = self.path.open("w", newline="", encoding="utf-8")
        self.writer = csv.writer(self.file)
        self.writer.writerow(
            (
                "timestep",
                "env",
                "episode",
                "episode_step",
                "seed",
                "observation",
                "action_mask",
                "action",
                "action_name",
                "reward",
                "terminated",
                "truncated",
            )
        )
        self.episodes = np.zeros(self.training_env.num_envs, dtype=int)
        self.steps = np.zeros(self.training_env.num_envs, dtype=int)

    def _on_step(self) -> bool:
        obs = self.locals["obs_tensor"]
        rows = zip(self.locals["actions"], self.locals["rewards"], self.locals["dones"], self.locals["infos"], self.locals["action_masks"])
        for env, (action, reward, done, info, mask) in enumerate(rows):
            action = int(np.asarray(action).item())
            truncated = bool(info.get("TimeLimit.truncated", False))
            self.writer.writerow(
                (
                    self.num_timesteps,
                    env,
                    self.episodes[env],
                    self.steps[env],
                    info["episode_seed"],
                    compact_json({key: value[env].cpu().numpy() for key, value in obs.items()}),
                    compact_json(mask),
                    action,
                    action_name(action),
                    float(reward),
                    bool(done and not truncated),
                    truncated,
                )
            )
            self.steps[env] += 1
            if done:
                self.episodes[env] += 1
                self.steps[env] = 0
        return True

    def _on_training_end(self) -> None:
        self.file.close()


def main() -> None:
    run_index = 0
    while (run_dir := Path("monitor") / f"PPO_{TOTAL_TIMESTEPS}_{run_index}").exists():
        run_index += 1
    run_dir.mkdir(parents=True)
    RUN_NAME = run_dir.name

    config = {
        "name": RUN_NAME,
        "algorithm": "MaskablePPO",
        "policy": "MultiInputPolicy",
        "total_timesteps": TOTAL_TIMESTEPS,
        "n_envs": N_ENVS,
        "training_seed": TRAINING_SEED,
        "evaluation_seed": EVALUATION_SEED,
        "evaluate_every_timesteps": EVALUATE_EVERY,
        "checkpoint_every_timesteps": CHECKPOINT_EVERY,
        "learning_rate": LEARNING_RATE,
        "learning_rate_schedule": "constant",
        "tensorboard_log": str(run_dir / "tensorboard"),
        "device": "cpu",
        "gamma": 1.0,
        "vec_env": VEC_ENV.__name__,
        "normalized_observations": True,
        "action_masking": True,
        "target_cookies": autocookie.TARGET_COOKIES,
        "episode_length": autocookie.EPISODE_LENGTH,
        "reward_mode": str(autocookie.REWARD_MODE),
    }
    (run_dir / "config.json").write_text(json.dumps(config, indent=2), encoding="utf-8")

    run = wandb.init(
        project="autocookie",
        name=RUN_NAME,
        config=config,
        sync_tensorboard=True,
        save_code=True,
    )

    train_env = make_vec_env(CookieEnv, N_ENVS, seed=TRAINING_SEED, wrapper_class=NormalizeCookieObservation, vec_env_cls=VEC_ENV, monitor_dir=str(run_dir))

    eval_env = make_vec_env(EvaluationEnv, wrapper_class=NormalizeCookieObservation, vec_env_cls=VEC_ENV)

    best_model = MaskableEvalCallback(
        eval_env,
        best_model_save_path=str(run_dir),
        log_path=str(run_dir),
        eval_freq=max(EVALUATE_EVERY // N_ENVS, 1),
        n_eval_episodes=1,
        deterministic=True,
    )

    model = MaskablePPO(
        "MultiInputPolicy",
        train_env,
        learning_rate=LEARNING_RATE,
        tensorboard_log=str(run_dir / "tensorboard"),
        device="cpu",
        seed=TRAINING_SEED,
        gamma=1.0,
        verbose=1,
    )

    model.learn(
        TOTAL_TIMESTEPS,
        tb_log_name=RUN_NAME,
        callback=[
            VerifyNormalizationCallback(),
            TrainingCsvCallback(run_dir / "training.csv"),
            CheckpointCallback(
                save_freq=max(CHECKPOINT_EVERY // N_ENVS, 1),
                save_path=str(run_dir / "checkpoints"),
                name_prefix=RUN_NAME,
            ),
            best_model,
            WandbCallback(gradient_save_freq=1_000),
        ],
    )

    train_env.close()
    eval_env.close()

    model = MaskablePPO.load(run_dir / "best_model", device="cpu")
    env = NormalizeCookieObservation(EvaluationEnv())
    observation, info = env.reset()
    terminated = truncated = False
    total_reward = 0.0

    with (run_dir / "evaluation.csv").open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(("step", "seed", "observation", "action_mask", "action", "action_name", "reward", "terminated", "truncated"))
        step = 0
        while not (terminated or truncated):
            mask = env.action_masks()
            action, _ = model.predict(observation, action_masks=mask, deterministic=True)
            action = int(np.asarray(action).item())
            next_observation, reward, terminated, truncated, info = env.step(action)
            writer.writerow(
                (
                    step,
                    info["episode_seed"],
                    compact_json(observation),
                    compact_json(mask),
                    action,
                    action_name(action),
                    reward,
                    terminated,
                    truncated,
                )
            )
            observation = next_observation
            total_reward += float(reward)
            step += 1

    run.summary.update(
        {
            "final_reward": total_reward,
            "final_steps": step,
            "terminated": terminated,
            "truncated": truncated,
        }
    )

    artifact = wandb.Artifact(RUN_NAME, type="run-output")
    artifact.add_dir(str(run_dir))
    run.log_artifact(artifact)
    run.finish()

    print(f"{RUN_NAME}: reward={total_reward:.6f}, steps={step}, best_model={run_dir / 'best_model.zip'}")
    env.close()


if __name__ == "__main__":
    main()

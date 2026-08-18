"""
python3 train.py --playback 2x
python3 train.py --playback 5x
python3 train.py --playback skip

default:
    seed = 10_000

python3 train.py --playback 5x --eval-seed 12345

Every run creates:
cookie_logs/artifacts/PPO_{TOTAL_TIMESTEPS}_DummyVecEnv_<timestamp>/
- run_config.json
- training_actions.csv
- evaluation_actions.csv
- evaluation_summary.json
- final_model.zip
- monitor/

training_actions.csv: every training action across all eight environments, with environment, episode, reward and terminal status.
evaluation_actions.csv: complete deterministic validation action sequence, cookies, CpS, buildings, upgrades, buffs and simulated time.
evaluation_summary.json: success/DNF, completion time, total reward and action counts.
monitor/: per-environment episode return and length.
final_model.zip: trained PPO model.
run_config.json: seeds, horizon, reward mode and training settings.

Requires:
    pip install sb3-contrib


"""

import argparse
import csv
import json
import os
from datetime import datetime
from pathlib import Path
from typing import TextIO
# os.environ["TORCH_COMPILE_DISABLE"] = "1"
# os.environ["TRITON_DISABLE"] = "1"

import torch
import triton

import numpy as np
import gymnasium as gym
from gymnasium.spaces import Discrete
from gymnasium.envs.registration import register
from stable_baselines3.common.callbacks import BaseCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.vec_env import SubprocVecEnv
from sb3_contrib import MaskablePPO

import cookie_env

from typing import Optional, Any

if "CookieClicker-v0" not in gym.registry:
    register(
        id="CookieClicker-v0",
        entry_point="cookie_env:CookieEnv",
    )

TOTAL_TIMESTEPS = 100_000
RUN_NAME = f"PPO_{TOTAL_TIMESTEPS}_DummyVecEnv"
TRAINING_SEED = 42
N_ENVS = 8



class VerifyNormalizationCallback(BaseCallback):
    def __init__(self):
        super().__init__()
        self.checked = False

    def _on_step(self) -> bool:
        if self.checked:
            return True

        observations = self.locals.get("new_obs")
        if observations is None:
            raise RuntimeError("PPO callback did not expose new_obs")

        action_masks = self.locals.get("action_masks")
        if action_masks is None:
            raise RuntimeError("MaskablePPO did not expose action masks")

        actions = np.asarray(self.locals["actions"]).reshape(-1)
        action_masks = np.asarray(action_masks, dtype=np.bool_)
        selected_actions_are_valid = action_masks[
            np.arange(actions.size),
            actions,
        ]
        if not np.all(selected_actions_are_valid):
            raise RuntimeError("MaskablePPO selected a masked action")

        valid_action_counts = action_masks.sum(axis=1)
        print(
            "Valid actions received by MaskablePPO: "
            f"min={valid_action_counts.min()}, "
            f"max={valid_action_counts.max()}"
        )

        normalized_keys = (
            "current_simulation_time",
            "current_cookies",
            "all_time_cookies",
            "handmade_cookies",
            "total_cps",
            "buildings_owned",
            "active_golden_cookie_buff_seconds_remaining",
        )

        print("Observations received by PPO:")

        for key in normalized_keys:
            values = np.asarray(observations[key])

            print(
                f"{key:48} "
                f"min={values.min():.6f}, max={values.max():.6f}"
            )

            if not np.all(np.isfinite(values)):
                raise RuntimeError(f"{key} contains non-finite values")

            if np.any(values < 0.0) or np.any(values > 1.0):
                raise RuntimeError(
                    f"{key} is not normalized: "
                    f"range [{values.min()}, {values.max()}]"
                )

        self.checked = True
        return True



def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--playback",
        choices=("2x", "5x", "skip"),
        default="2x",
        help="Speed of the post-training evaluation playback.",
    )
    parser.add_argument(
        "--eval-seed",
        type=int,
        default=10_000,
        help="Seed for the fresh post-training evaluation episode.",
    )
    return parser.parse_args()


def action_name(action: int) -> str:
    if action == 0:
        return "Advance"

    if action < cookie_env.autocookie.UPGRADE_ACTION_OFFSET:
        purchase_index = action - 1
        quantity_count = len(cookie_env.CookieEnv.buying_quantities)
        building_index = purchase_index // quantity_count
        quantity_index = purchase_index % quantity_count
        building_name = cookie_env.CookieEnv.building_names[building_index]
        quantity = cookie_env.CookieEnv.buying_quantities[quantity_index]
        suffix = "" if quantity == 1 else "s"
        return f"Buy {quantity} {building_name}{suffix}"

    upgrade_index = action - cookie_env.autocookie.UPGRADE_ACTION_OFFSET
    return f"Buy {cookie_env.CookieEnv.upgrade_names[upgrade_index]}"


class TrainingActionTraceCallback(BaseCallback):
    """Stream every training action to CSV without retaining it in memory."""

    def __init__(self, output_path: Path):
        super().__init__()
        self.output_path = output_path
        self.output_file: TextIO | None = None
        self.writer: csv.DictWriter | None = None
        self.episode_indices: list[int] = []
        self.episode_steps: list[int] = []

    def _on_training_start(self) -> None:
        output_file = self.output_path.open(
            "w",
            newline="",
            encoding="utf-8",
        )
        writer = csv.DictWriter(
            output_file,
            fieldnames=(
                "transition_index",
                "vector_step",
                "global_timestep",
                "env_index",
                "episode_index",
                "episode_step",
                "action_index",
                "action_name",
                "reward",
                "done",
                "terminated",
                "truncated",
                "episode_return",
                "episode_length",
            ),
        )
        writer.writeheader()
        self.output_file = output_file
        self.writer = writer
        self.episode_indices = [0] * self.training_env.num_envs
        self.episode_steps = [0] * self.training_env.num_envs

    def _on_step(self) -> bool:
        writer = self.writer
        output_file = self.output_file
        if writer is None or output_file is None:
            raise RuntimeError("training action trace is not open")

        actions = np.asarray(self.locals["actions"]).reshape(-1)
        rewards = np.asarray(self.locals["rewards"]).reshape(-1)
        dones = np.asarray(self.locals["dones"]).reshape(-1)
        infos = self.locals["infos"]

        for env_index, (action, reward, done, info) in enumerate(
            zip(actions, rewards, dones, infos)
        ):
            action = int(action)
            truncated = bool(info.get("TimeLimit.truncated", False))
            terminated = bool(done and not truncated)
            episode_info = info.get("episode", {})
            writer.writerow(
                {
                    "transition_index": (
                        (self.n_calls - 1) * self.training_env.num_envs
                        + env_index
                    ),
                    "vector_step": self.n_calls,
                    "global_timestep": self.num_timesteps,
                    "env_index": env_index,
                    "episode_index": self.episode_indices[env_index],
                    "episode_step": self.episode_steps[env_index],
                    "action_index": action,
                    "action_name": action_name(action),
                    "reward": float(reward),
                    "done": bool(done),
                    "terminated": terminated,
                    "truncated": truncated,
                    "episode_return": episode_info.get("r", ""),
                    "episode_length": episode_info.get("l", ""),
                }
            )

            self.episode_steps[env_index] += 1
            if done:
                self.episode_indices[env_index] += 1
                self.episode_steps[env_index] = 0

        if self.n_calls % 100 == 0:
            output_file.flush()
        return True

    def _on_training_end(self) -> None:
        if self.output_file is not None:
            self.output_file.close()
            self.output_file = None


def run_evaluation(
    model: MaskablePPO,
    artifact_directory: Path,
    eval_seed: int,
    playback: str,
) -> None:
    render_enabled = playback != "skip"
    render_speed = float(playback.removesuffix("x")) if render_enabled else 1.0
    render_mode = "human" if render_enabled else None

    raw_test_env = gym.make(
        "CookieClicker-v0",
        render_mode=render_mode,
        render_speed=render_speed,
    )
    test_env = cookie_env.NormalizeCookieObservation(raw_test_env)
    observation, info = test_env.reset(seed=eval_seed)
    raw_observation = test_env.last_raw_observation
    if raw_observation is None:
        test_env.close()
        raise RuntimeError("evaluation reset did not produce an observation")
    print("Initial evaluation observation:", raw_observation)

    terminated = False
    truncated = False
    total_reward = 0.0
    step_index = 0
    action_space = test_env.action_space
    if not isinstance(action_space, Discrete):
        test_env.close()
        raise TypeError("CookieClicker-v0 must use a Discrete action space")
    action_counts = np.zeros(int(action_space.n), dtype=np.int64)
    trace_path = artifact_directory / "evaluation_actions.csv"

    with trace_path.open("w", newline="", encoding="utf-8") as trace_file:
        writer = csv.DictWriter(
            trace_file,
            fieldnames=(
                "step",
                "action_index",
                "action_name",
                "reward",
                "cumulative_reward",
                "simulation_time",
                "delta_simulation_time",
                "current_cookies",
                "all_time_cookies",
                "handmade_cookies",
                "total_cps",
                "buildings_owned",
                "upgrades_owned",
                "active_buffs",
                "terminated",
                "truncated",
            ),
        )
        writer.writeheader()

        while not (terminated or truncated):
            previous_time = float(
                raw_observation["current_simulation_time"][0]
            )
            action_mask = test_env.action_masks()
            action, _state = model.predict(
                observation,
                action_masks=action_mask,
                deterministic=True,
            )
            action = int(np.asarray(action).item())
            if not bool(action_mask[action]):
                raise RuntimeError(
                    "MaskablePPO selected a masked evaluation action"
                )

            observation, reward, terminated, truncated, info = test_env.step(
                action
            )
            raw_observation = test_env.last_raw_observation
            if raw_observation is None:
                raise RuntimeError(
                    "evaluation step did not produce an observation"
                )
            total_reward += float(reward)
            action_counts[action] += 1

            simulation_time = float(
                raw_observation["current_simulation_time"][0]
            )
            writer.writerow(
                {
                    "step": step_index,
                    "action_index": action,
                    "action_name": action_name(action),
                    "reward": float(reward),
                    "cumulative_reward": total_reward,
                    "simulation_time": simulation_time,
                    "delta_simulation_time": simulation_time - previous_time,
                    "current_cookies": float(
                        raw_observation["current_cookies"][0]
                    ),
                    "all_time_cookies": float(
                        raw_observation["all_time_cookies"][0]
                    ),
                    "handmade_cookies": float(
                        raw_observation["handmade_cookies"][0]
                    ),
                    "total_cps": float(raw_observation["total_cps"][0]),
                    "buildings_owned": json.dumps(
                        raw_observation["buildings_owned"].tolist()
                    ),
                    "upgrades_owned": json.dumps(
                        np.flatnonzero(
                            raw_observation["upgrades_owned"]
                        ).tolist()
                    ),
                    "active_buffs": json.dumps(
                        np.flatnonzero(
                            raw_observation[
                                "active_golden_cookie_buffs"
                            ]
                        ).tolist()
                    ),
                    "terminated": bool(terminated),
                    "truncated": bool(truncated),
                }
            )

            if render_enabled:
                test_env.render()
            step_index += 1

    summary = {
        "evaluation_seed": eval_seed,
        "playback": playback,
        "terminated": bool(terminated),
        "truncated": bool(truncated),
        "outcome": "success" if terminated else "DNF",
        "steps": step_index,
        "total_reward": total_reward,
        "final_simulation_time": float(
            raw_observation["current_simulation_time"][0]
        ),
        "final_all_time_cookies": float(
            raw_observation["all_time_cookies"][0]
        ),
        "action_counts": {
            action_name(index): int(count)
            for index, count in enumerate(action_counts)
        },
    }
    with (artifact_directory / "evaluation_summary.json").open(
        "w",
        encoding="utf-8",
    ) as summary_file:
        json.dump(summary, summary_file, indent=2)

    print(
        f"Evaluation ended with {summary['outcome']} after "
        f"{summary['final_simulation_time']:.3f} simulated seconds; "
        f"total reward: {total_reward:.6f}"
    )
    print(f"Saved run artifacts to: {artifact_directory}")
    test_env.close()


def main():
    args = parse_args()
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    artifact_directory = (
        Path("cookie_logs") / "artifacts" / f"{RUN_NAME}_{timestamp}"
    )
    artifact_directory.mkdir(parents=True, exist_ok=False)

    run_config = {
        "run_name": RUN_NAME,
        "training_seed": TRAINING_SEED,
        "evaluation_seed": args.eval_seed,
        "number_of_environments": N_ENVS,
        "total_training_timesteps": TOTAL_TIMESTEPS,
        "ppo_gamma": 1.0,
        "episode_length_seconds": cookie_env.autocookie.EPISODE_LENGTH,
        "target_cookies": cookie_env.autocookie.TARGET_COOKIES,
        "reward_mode": str(cookie_env.autocookie.REWARD_MODE),
        "algorithm": "MaskablePPO",
        "action_masking": True,
        "observation_normalization": (
            "NormalizeCookieObservation fixed transforms"
        ),
        "playback": args.playback,
    }
    with (artifact_directory / "run_config.json").open(
        "w",
        encoding="utf-8",
    ) as config_file:
        json.dump(run_config, config_file, indent=2)

    # train_env = gym.make("CookieClicker-v0")
    train_env = make_vec_env(
        "CookieClicker-v0",
        n_envs=N_ENVS,
        seed=TRAINING_SEED,
        env_kwargs={"render_mode": None},
        wrapper_class=cookie_env.NormalizeCookieObservation,
        monitor_dir=str(artifact_directory / "monitor"),
    )

       
    # train_env = make_vec_env(
    #     "CookieClicker-v0",
    #     n_envs=8,
    #     seed=42,
    #     env_kwargs={"render_mode": None},
    #     wrapper_class=cookie_env.NormalizeCookieObservation,
    #     vec_env_cls=SubprocVecEnv,
    # )

    model = MaskablePPO(
        "MultiInputPolicy",
        train_env,
        verbose=1,
        device="cpu",
        tensorboard_log="./cookie_logs/",
        seed=TRAINING_SEED,
        gamma=1.0,
    )
    print("STARTED TRAINING")
    
    # model.learn(
    #     total_timesteps=TOTAL_TIMESTEPS,
    #     tb_log_name=RUN_NAME,
    #     callback=TrainingActionTraceCallback(
    #         artifact_directory / "training_actions.csv"
    #     ),
    # )  # 250_000 for real training
    
    model.learn(
        total_timesteps=TOTAL_TIMESTEPS,
        tb_log_name=RUN_NAME,
        callback=[
            TrainingActionTraceCallback(
                artifact_directory / "training_actions.csv"
            ),
            VerifyNormalizationCallback(),
        ],
    )
    
    
    print("FINISHED TRAINING")
    model.save(artifact_directory / "final_model")
    train_env.close()

    run_evaluation(
        model,
        artifact_directory,
        eval_seed=args.eval_seed,
        playback=args.playback,
    )


if __name__ == "__main__":
    main()
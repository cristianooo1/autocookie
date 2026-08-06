import sys
import os

sys.path.append(os.path.abspath("build"))

import autocookie

def main():
    env = autocookie.Env()

    initial_obs = env.reset()
    print(f"initial cookies: {initial_obs.current_cookies}")
    print(f"initial all time cookies: {initial_obs.all_time_cookies}")
    print(f"initial CPS: {initial_obs.cps}")


    # for building in autocookie.BuildingType:
    #     print(f"name={building.name} and value = {building.value}")
        
    # for action in autocookie.ActionType:
    #     print(f"name={action.name} and value = {action.value}")
            
    click_action = autocookie.Action()
    click_action.type = autocookie.ActionType.ClickCookie

    buy_cursor_action = autocookie.Action()
    buy_cursor_action.type = autocookie.ActionType.BuyBuilding
    buy_cursor_action.buildingIndex = autocookie.BuildingType.CURSOR

    wait_action = autocookie.Action()
    wait_action.type = autocookie.ActionType.Wait

    done = False
    step_num = 1
    total_reward = 0
    

    while not done and step_num <= 200:
        
        if step_num <= 3:
            action = click_action
        elif step_num == 4:
            action = buy_cursor_action
        else:
            action = wait_action

        result = env.step(action)
        done = result.done
        obs = result.obs
        
        print(f"##### Step {step_num} ########")
        print(f"cookies: {obs.current_cookies:.2f}")
        print(f"CPS: {obs.cps:.2f}")
        print(f"buildings owned: {obs.buildings_owned}") 
        print(f"reward: {result.reward:.2f}")
        total_reward +=result.reward
        print(f"total reward: {total_reward:.2f}")
        print(f"Terminal?: {result.done}\n")
        
        step_num += 1

if __name__ == "__main__":
    main()

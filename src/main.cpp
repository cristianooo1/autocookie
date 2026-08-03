#include <iostream>
#include <string>
#include <chrono>
#include <tuple>
#include <vector>
#include <memory> //for std::unique_ptr
#include <cmath>
#include <string>
#include <array>

// for UI and user inputs
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

/*
SIMULATION STEPS:

Current state

Receive action = PLAYER COMMAND

Execute action BY ENGINE

Events occur

Recalculate production

Advance timers - simulation by dt

Return new state
*/

/*
RL TRAINING:
reset()

step(action)

getObservation()

getReward()

isTerminal()
*/

enum class BuildingType
{
    CURSOR = 0,
    GRANDMA = 1,
    FARM = 2,
};

enum class ActionType
{
    ClickCookie,
    BuyBuilding,
    Wait,
    // ClickGoldenCookie,
    // Ascend,

};

// https://www.learncpp.com/cpp-tutorial/scoped-enumerations-enum-classes/
template <typename T>
constexpr auto operator+(T a) noexcept
{
    return static_cast<std::underlying_type_t<T>>(a);
}

struct BuildingDefinition
{
    double base_cost{0.1};
    double base_production{0.1};
    double cost_multiplier{0.1};
};

struct Action
{
    ActionType type{};
    BuildingType buildingIndex{};
};

namespace Config
{
    constexpr int TOTAL_NR_BUILDINGS_AVAILABLE = 3;
    std::array<BuildingDefinition, TOTAL_NR_BUILDINGS_AVAILABLE> buildingsDefinitions{
        BuildingDefinition{
            // CURSOR = 0
            .base_cost = 1,
            .base_production = 0.1,
            .cost_multiplier = 1.15,
        },
        BuildingDefinition{
            // GRANDMA = 1
            .base_cost = 20,
            .base_production = 0.25,
            .cost_multiplier = 1.25,
        },
        BuildingDefinition{
            // FARM = 2
            .base_cost = 50,
            .base_production = 0.4,
            .cost_multiplier = 1.5,
        }};
};

struct GameState
{

    /*
    0 -> cursor
    1 -> grandma
    2 -> farm
    */
    std::array<int, Config::TOTAL_NR_BUILDINGS_AVAILABLE> buildingsOwned{
        0, // number of CURSORS owned
        0, // number of GRANDMAS owned
        0, // number of FARMS owned
    };

    double current_cookies{0.0};
    double alltime_cookies{0.0};
    double cps{0.0};

    double time{0.0};
    // active_buffs
    // upgrades_owned
};

struct BuildingSystem
{

    void update(GameState &state, const Action &action)
    {
        double price = Config::buildingsDefinitions[+action.buildingIndex].base_cost *
                       std::pow(
                           Config::buildingsDefinitions[+action.buildingIndex].cost_multiplier,
                           static_cast<double>(state.buildingsOwned[+action.buildingIndex]));

        if (action.type == ActionType::BuyBuilding && state.current_cookies >= price)
        {
            state.current_cookies -= price;
            state.buildingsOwned[+action.buildingIndex] += 1;
        }
        else if (action.type == ActionType::ClickCookie)
        {
            state.current_cookies += 1;
            state.alltime_cookies += 1;
        }
        else if (action.type == ActionType::Wait)
        {
        }
    }
};

struct Economy
{
    void update(GameState &state, const double dt)
    {
        double base_cps = 0.0;
        for (int i = 0; i < Config::TOTAL_NR_BUILDINGS_AVAILABLE; i++)
        {
            base_cps += static_cast<double>(state.buildingsOwned[i]) * Config::buildingsDefinitions[i].base_production;
        };
        state.cps = base_cps; // * upgrades * buffs * ... TO BE IMPLEMENTED!!!!!!!!
        state.current_cookies += state.cps * dt;
        state.alltime_cookies += state.cps * dt;
    }
};

struct Engine
{
    const double dt{1.0};
    GameState state;
    Economy economy;
    BuildingSystem buildingSystem;

    void step(const Action &action)
    {
        buildingSystem.update(state, action);
        state.time += dt;
        // events.update(state, dt);
        economy.update(state, dt);
    }

    std::tuple<double, double, double, double, int, int, int> queryState()
    {
        // std::cout << "Current Cookies: " << this->state.current_cookies << "\n";
        // std::cout << "All Time Cookies: " << this->state.alltime_cookies << "\n";
        // std::cout << "CPS: " << this->state.cps << "\n";
        // std::cout << "Cursors: " << this->state.buildingsOwned[+BuildingType::CURSOR] << "\n";
        // std::cout << "Grandmas: " << this->state.buildingsOwned[+BuildingType::GRANDMA] << "\n";
        // std::cout << "Farms: " << this->state.buildingsOwned[+BuildingType::FARM] << "\n";
        return std::make_tuple(
            state.current_cookies,
            state.alltime_cookies,
            state.cps,
            state.time,
            state.buildingsOwned[+BuildingType::CURSOR],
            state.buildingsOwned[+BuildingType::GRANDMA],
            state.buildingsOwned[+BuildingType::FARM]);
    }
};

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char *argv[])
{
    Engine engine;

    Action clickCookie{.type = ActionType::ClickCookie};
    Action buyCursor{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::CURSOR};
    Action buyGrandma{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::GRANDMA};
    Action buyFarm{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::FARM};
    Action justWait{.type = ActionType::Wait};

    // engine.step(buyCursor);
    // engine.step(buyFarm);
    // engine.step(clickCookie);
    // engine.queryState();

    // GLFW + Dear ImGUI
    // https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp
    if (!glfwInit())
    {
        std::cout << "GLFW Initialization failed" << "\n";
        exit(EXIT_FAILURE);
    }

    // GL version + backend selects GLSL version
    const char *glsl_version = nullptr;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "cookie test math model", nullptr, nullptr);
    if (window == nullptr)
    {
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback); // keyboard input
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true); // pointer to the window created by GLFW
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool show_this_bool = true;
    bool show_other_bool = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {

            ImGui::Begin("cookie clicker stats");

            ImGui::Text("statistics:"); // Display some text (you can use a format strings too)
            // ImGui::Checkbox("Demo Window", &show_this_bool); // Edit bools storing our window open/close state
            // ImGui::Checkbox("Another Window", &show_other_bool);

            // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
            // ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("COOKIE"))
            {
                engine.step(clickCookie);
            }

            ImGui::SameLine();

            if (ImGui::Button("buy cursor"))
            {
                engine.step(buyCursor);
            }

            ImGui::SameLine();

            if (ImGui::Button("buy grandma"))
            {
                engine.step(buyGrandma);
            }

            ImGui::SameLine();

            if (ImGui::Button("buy farm"))
            {
                engine.step(buyFarm);
            }

            ImGui::SameLine();

            if (ImGui::Button("wait"))
            {
                engine.step(justWait);
            }

            const auto [currCookiess, alltimeCookiess, CPSs, time_from_startt, cursorss, grandmass, farmss] = engine.queryState();

            ImGui::Text("current Cookies = %.3f", currCookiess);

            ImGui::Text("alltime Cookies = %.3f", alltimeCookiess);

            ImGui::Text("CPS = %.3f", CPSs);

            ImGui::Text("time from start = %.3f", time_from_startt);

            ImGui::Text("cursors = %i", cursorss);

            ImGui::Text("grandmas = %i", grandmass);

            ImGui::Text("farms = %i", farmss);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

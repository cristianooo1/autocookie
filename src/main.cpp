#include "env.hpp"

// for UI and user inputs
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char *argv[])
{
    Env env;
    bool interactive_mode = false;

    Action clickCookie{.type = ActionType::ClickCookie};
    Action buyCursor{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::CURSOR};
    Action buyGrandma{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::GRANDMA};
    Action buyFarm{.type = ActionType::BuyBuilding, .buildingIndex = BuildingType::FARM};
    Action justWait{.type = ActionType::Wait};

    if (interactive_mode != 0)
    {
        env.step(buyCursor);
        env.step(buyFarm);
        env.step(clickCookie);

        const auto [currCookiess, alltimeCookiess, CPSs, time_from_startt, cursorss, grandmass, farmss] = env.queryState();

        std::cout << "current Cookies = " << currCookiess << "\n";
        std::cout << "alltime Cookies = " << alltimeCookiess << "\n";
        std::cout << "cps = " << CPSs << "\n";
        std::cout << "time from start = " << time_from_startt << "\n";
        std::cout << "cursros = " << cursorss << "\n";
        std::cout << "grandmas = " << grandmass << "\n";
        std::cout << "farms = " << farmss << "\n";
    }
    else
    {
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
                    env.step(clickCookie);
                }

                ImGui::SameLine();

                if (ImGui::Button("buy cursor"))
                {
                    env.step(buyCursor);
                }

                ImGui::SameLine();

                if (ImGui::Button("buy grandma"))
                {
                    env.step(buyGrandma);
                }

                ImGui::SameLine();

                if (ImGui::Button("buy farm"))
                {
                    env.step(buyFarm);
                }

                ImGui::SameLine();

                if (ImGui::Button("wait"))
                {
                    env.step(justWait);
                }

                ImGui::SameLine();

                if (ImGui::Button("reset"))
                {
                    env.reset();
                }

                const auto [currCookiess, alltimeCookiess, CPSs, time_from_startt, cursorss, grandmass, farmss] = env.queryState();

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
    }

    return 0;
}

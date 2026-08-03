cmake_minimum_required(VERSION 3.15)
project(autocookie)

#  modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# dear ImGUI
# exclude tests/examples
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

find_package(glfw QUIET)
if (NOT glfw_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(glfw)
endif()

# dear ImGUI
find_package(imgui QUIET)
if (NOT imgui_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG master
    )
    FetchContent_MakeAvailable(imgui)
endif()


# # raylib
# find_package(raylib QUIET)
# if (NOT raylib_FOUND)
#     include(FetchContent)
#     FetchContent_Declare(
#         raylib
#         GIT_REPOSITORY https://github.com/raysan5/raylib.git
#         GIT_TAG 6.0
#         GIT_SHALLOW 1
#     )
#     FetchContent_MakeAvailable(raylib)
# endif()

# # raylib cpp
# find_package(raylib_cpp QUIET)
# if (NOT raylib_cpp_FOUND)
#     if (NOT DEFINED RAYLIB_CPP_VERSION)
#         set(RAYLIB_CPP_VERSION v6.0.0)
#     endif()
#     include(FetchContent)
#     FetchContent_Declare(
#         raylib_cpp
#         GIT_REPOSITORY https://github.com/RobLoach/raylib-cpp.git
#         GIT_TAG ${RAYLIB_CPP_VERSION}
#     )
#     FetchContent_MakeAvailable(raylib_cpp)
# endif()

# # rl_tools
# find_package(rl_tools QUIET)
# if (NOT rl_tools_FOUND)
#     include(FetchContent)
#     FetchContent_Declare(
#         rl_tools
#         GIT_REPOSITORY https://github.com/rl-tools/rl-tools.git
#         GIT_TAG main
#     )
#     FetchContent_MakeAvailable(rl_tools)
# endif()




add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    # backends: GLFW + OpenGL3
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PRIVATE glfw)

find_package(OpenGL REQUIRED)

set(CORE_SOURCES 
    # src/car.cpp
    # src/game.cpp
    # src/obstacle.cpp
    # src/transform2D.cpp
    # src/boundingBox2D.cpp
    # src/mapGenerator.cpp
    # src/rayCasting.cpp
)

# STANDARD GAME
add_executable(${PROJECT_NAME} src/main.cpp ${CORE_SOURCES})

# where CMake looks for .hpp files
target_include_directories(${PROJECT_NAME} PRIVATE include)


# link libraries to main program
target_link_libraries(${PROJECT_NAME} PRIVATE imgui glfw OpenGL::GL)
# target_link_libraries(${PROJECT_NAME} PUBLIC raylib raylib_cpp RLtools::RLtools)

# RL TRAINER
# add_executable(${PROJECT_NAME}_train src/train.cpp ${CORE_SOURCES})
# target_include_directories(${PROJECT_NAME}_train PRIVATE include)
# target_link_libraries(${PROJECT_NAME}_train PUBLIC raylib raylib_cpp RLtools::RLtools)

# Switch from DEBUG to RELEASE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_options(${PROJECT_NAME}_train PRIVATE -Ofast)
#     if(NOT APPLE)
#         target_compile_options(${PROJECT_NAME}_train PRIVATE -march=native)
#     endif()
# endif()
cmake_minimum_required(VERSION 3.15)
project(autocookie)

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.9 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

#  modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)

# glfw
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
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()
endif()

# nanobind
find_package(nanobind QUIET)
if (NOT nanobind_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind
        GIT_TAG master
    )
    FetchContent_MakeAvailable(nanobind)
endif()

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
target_link_libraries(imgui PRIVATE glfw OpenGL::GL)

set(CORE_SOURCES 
    src/env.cpp
    src/buildingSystem.cpp
    src/economySystem.cpp
    src/simulationSystem.cpp
    src/eventSystem.cpp   
)

# CORE
add_library(${PROJECT_NAME}_core STATIC ${CORE_SOURCES})
target_include_directories(${PROJECT_NAME}_core PUBLIC include)
target_link_libraries(${PROJECT_NAME}_core PUBLIC imgui glfw OpenGL::GL)

# STANDARD GAME 
add_executable(${PROJECT_NAME}_main src/main.cpp)
target_link_libraries(${PROJECT_NAME}_main PRIVATE ${PROJECT_NAME}_core)

# TESTS
include(CTest)

if(BUILD_TESTING)
    add_executable(
        ${PROJECT_NAME}_tests
        tests/cookieLogicTests.cpp
    )

    target_link_libraries(
        ${PROJECT_NAME}_tests
        PRIVATE ${PROJECT_NAME}_core
    )

    add_test(
        NAME ${PROJECT_NAME}_logic_tests
        COMMAND ${PROJECT_NAME}_tests
    )
endif()

# NANOBIND
# nanobind_add_module(${PROJECT_NAME} src/bindings.cpp)
# target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
# set_target_properties(${PROJECT_NAME} PROPERTIES 
#     LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# )
# nanobind_add_stub(
#     ${PROJECT_NAME}_stub
#     MODULE ${PROJECT_NAME}
#     OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pyi
#     PYTHON_PATH ${CMAKE_CURRENT_SOURCE_DIR}
#     DEPENDS ${PROJECT_NAME}
# )
# add_custom_target(generate_stubs ALL DEPENDS ${PROJECT_NAME}_stub)

# Switch from DEBUG to RELEASE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_options(${PROJECT_NAME}_train PRIVATE -Ofast)
#     if(NOT APPLE)
#         target_compile_options(${PROJECT_NAME}_train PRIVATE -march=native)
#     endif()
# endif()
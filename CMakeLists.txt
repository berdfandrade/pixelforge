cmake_minimum_required(VERSION 3.24)
project(PixelForge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS CONFIG
)

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
    GIT_SHALLOW TRUE
    GIT_SUBMODULES external/freetype
    FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(SDL3 SDL3_ttf)

add_library(pixelforge_engine STATIC
    src/App.cpp
    src/core/Theme.cpp
    src/core/FileWatcher.cpp
    src/core/Log.cpp
    src/platform/Input.cpp
    src/render/BitmapFont.cpp
    src/render/Renderer.cpp
    src/render/TextRenderer.cpp
    src/render/SpriteLibrary.cpp
    src/scene/Scene.cpp
    src/scene/SceneSerializer.cpp
    src/game/Physics.cpp
    src/game/Gameplay.cpp
    src/game/GameRenderer.cpp
    src/editor/Ui.cpp
    src/editor/Editor.cpp
    src/editor/Hierarchy.cpp
    src/editor/Inspector.cpp
    src/editor/Viewport.cpp
    src/editor/BottomPanel.cpp
)
target_include_directories(pixelforge_engine PUBLIC src)
target_link_libraries(pixelforge_engine PUBLIC SDL3_ttf::SDL3_ttf SDL3::SDL3)
target_compile_definitions(pixelforge_engine PUBLIC PF_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
target_compile_options(pixelforge_engine PUBLIC $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

add_executable(pixelforge src/main.cpp)
target_link_libraries(pixelforge PRIVATE pixelforge_engine)

enable_testing()
add_executable(pixelforge_tests tests/GameplayTests.cpp)
target_link_libraries(pixelforge_tests PRIVATE pixelforge_engine)
add_test(NAME gameplay COMMAND pixelforge_tests)

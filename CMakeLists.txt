cmake_minimum_required(VERSION 3.20)
project(HomemadeRasterizer)

# Download SDL3 directly from official GitHub
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0 
)
FetchContent_MakeAvailable(SDL3)

# Compile main.cpp into an executable named "rasterizer"
add_executable(rasterizer main.cpp)

# Link SDL3 (This fixes your #include error automatically)
target_link_libraries(rasterizer PRIVATE SDL3::SDL3)

# Automatically copy required DLLs to the executable folder after building
add_custom_command(TARGET rasterizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:rasterizer>
    $<TARGET_FILE_DIR:rasterizer>
    COMMAND_EXPAND_LISTS
)
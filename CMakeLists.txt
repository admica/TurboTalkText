cmake_minimum_required(VERSION 3.10)

# For Windows using MinGW
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER /usr/bin/x86_64-w64-mingw32-windres)

project(TurboTalkText)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure whisper.cpp build options
set(BUILD_SHARED_LIBS OFF)
set(WHISPER_BUILD_TESTS OFF)
set(WHISPER_BUILD_EXAMPLES OFF)
set(WHISPER_BUILD_SERVER OFF)
set(WHISPER_SDL2 OFF)
set(WHISPER_CCACHE OFF)

# Build whisper.cpp
add_subdirectory(${CMAKE_SOURCE_DIR}/whisper.cpp)

# Add spdlog as a submodule (assuming it's in external/spdlog)
add_subdirectory(external/spdlog)

# ImGui sources
set(IMGUI_SOURCES
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_impl_sdl2.cpp
    src/imgui/imgui_impl_sdlrenderer2.cpp
)

# Add executable
add_executable(TurboTalkText
    src/main.cpp
    src/logger.cpp
    src/gui.cpp
    ${IMGUI_SOURCES}
)
set_target_properties(TurboTalkText PROPERTIES SUFFIX ".exe")

# SDL3 setup (assuming SDL3; adjust to SDL2 if needed)
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/SDL3-mingw)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL3_DIR}/x86_64-w64-mingw32/include
    ${CMAKE_SOURCE_DIR}/whisper.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui
)

# Link libraries
target_link_libraries(TurboTalkText 
    ${SDL3_DIR}/x86_64-w64-mingw32/lib/libSDL3.dll.a
    whisper
    user32
    spdlog::spdlog
)

# Copy settings.json to output directory
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/settings.json"
        "$<TARGET_FILE_DIR:TurboTalkText>/settings.json"
    COMMENT "Copying settings.json to output directory"
)

# Copy SDL3.dll to output directory
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${SDL3_DIR}/x86_64-w64-mingw32/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:TurboTalkText>/SDL3.dll"
    COMMENT "Copying SDL3.dll to output directory"
)

# Copy MinGW runtime DLLs to output directory
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgcc_s_seh-1.dll"
        "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libstdc++-6.dll"
        "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
        "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgomp-1.dll"
        "$<TARGET_FILE_DIR:TurboTalkText>/"
    COMMENT "Copying MinGW runtime DLLs to output directory"
)

# Copy Whisper model file to output directory
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/whisper.cpp/models/ggml-base.en.bin"
        "$<TARGET_FILE_DIR:TurboTalkText>/ggml-base.en.bin"
    COMMENT "Copying Whisper model file to output directory"
)

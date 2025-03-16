cmake_minimum_required(VERSION 3.10)

# For Windows using MinGW
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER /usr/bin/x86_64-w64-mingw32-windres)

project(TurboTalkText)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure whisper.cpp
set(BUILD_SHARED_LIBS OFF)
set(WHISPER_BUILD_TESTS OFF)
set(WHISPER_BUILD_EXAMPLES OFF)
set(WHISPER_BUILD_SERVER OFF)
set(WHISPER_SDL2 OFF)
set(WHISPER_CCACHE OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/whisper.cpp)

# ImGui sources
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

# Executable
add_executable(TurboTalkText
    src/main.cpp
    src/logger.cpp
    src/gui.cpp
    ${IMGUI_SOURCES}
)
set_target_properties(TurboTalkText PROPERTIES SUFFIX ".exe")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/spdlog/include
    ${CMAKE_SOURCE_DIR}/src/imgui
    ${SDL2_DIR}/x86_64-w64-mingw32/include
    ${CMAKE_SOURCE_DIR}/whisper.cpp
)

# Link libraries
target_link_libraries(TurboTalkText 
    ${SDL2_DIR}/x86_64-w64-mingw32/lib/libSDL2.dll.a
    whisper
    user32
)

# Copy files to output directory
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/settings.json"
        "$<TARGET_FILE_DIR:TurboTalkText>/settings.json"
    COMMENT "Copying settings.json"
)
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${SDL2_DIR}/x86_64-w64-mingw32/bin/SDL2.dll"
        "$<TARGET_FILE_DIR:TurboTalkText>/SDL2.dll"
    COMMENT "Copying SDL2.dll"
)
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll"
        "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll"
        "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
        "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgomp-1.dll"
        "$<TARGET_FILE_DIR:TurboTalkText>/"
    COMMENT "Copying MinGW runtime DLLs"
)
add_custom_command(TARGET TurboTalkText POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/whisper.cpp/models/ggml-base.en.bin"
        "$<TARGET_FILE_DIR:TurboTalkText>/ggml-base.en.bin"
    COMMENT "Copying Whisper model"
)

cmake_minimum_required(VERSION 3.10)
project(TurboTalkText)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SYSTEM_NAME Windows)
add_definitions(-DSDL_MAIN_HANDLED)

# Source files
set(SOURCES
    src/main_nogui.cpp
    src/audio_manager.cpp
    src/transcription.cpp
    src/keyboard.cpp
    src/hotkey.cpp
    src/settings.cpp
    src/logger.cpp
)

# Static linking
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(WHISPER_STATIC ON CACHE BOOL "Build whisper static" FORCE)
set(GGML_STATIC ON CACHE BOOL "Build ggml static" FORCE)

# SDL2
set(SDL2_DIR ${CMAKE_SOURCE_DIR}/SDL2-mingw/x86_64-w64-mingw32)
find_library(SDL2_LIBRARY NAMES SDL2 PATHS ${SDL2_DIR}/lib)
include_directories(${SDL2_DIR}/include)

# Whisper.cpp
set(WHISPER_DIR ${CMAKE_SOURCE_DIR}/whisper.cpp)
set(GGML_OPENMP OFF CACHE BOOL "Disable OpenMP" FORCE)
add_subdirectory(${WHISPER_DIR} whisper_build)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${WHISPER_DIR}
    ${WHISPER_DIR}/ggml/include
    ${WHISPER_DIR}/ggml/src
    ${SDL2_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/spdlog/include
    ${CMAKE_SOURCE_DIR}/include/nlohmann
)

# Libraries
set(LIBRARIES
    ${SDL2_LIBRARY}
    whisper
    ggml
    setupapi ole32 oleaut32 imm32 version winmm gdi32 cfgmgr32 user32 mingw32
)

# Executable
add_executable(TurboTalkText-console ${SOURCES})
target_link_libraries(TurboTalkText-console PRIVATE mingw32 ${LIBRARIES})
target_link_options(TurboTalkText-console PRIVATE -mconsole)

# Copy SDL2.dll
if(WIN32)
    add_custom_command(TARGET TurboTalkText-console POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_DIR}/bin/SDL2.dll"
        $<TARGET_FILE_DIR:TurboTalkText-console>)
endif()

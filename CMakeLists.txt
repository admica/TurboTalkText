cmake_minimum_required(VERSION 3.10)
project(TurboTalkText)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Ensure we have SDL_main definition
add_definitions(-DSDL_MAIN_HANDLED)

# Define source files
set(SOURCES
    src/main_nogui.cpp
    src/logger.cpp
    src/settings.cpp
)

# Force static library builds for whisper and ggml
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(WHISPER_STATIC ON CACHE BOOL "Build whisper static libraries" FORCE)
set(GGML_STATIC ON CACHE BOOL "Build ggml static libraries" FORCE)

set(SDL2_DIR ${CMAKE_SOURCE_DIR}/SDL2-mingw/x86_64-w64-mingw32)
find_library(SDL2_LIBRARY NAMES SDL2 PATHS ${SDL2_DIR}/lib)
include_directories(${SDL2_DIR}/include)

set(WHISPER_DIR ${CMAKE_SOURCE_DIR}/whisper.cpp)
set(GGML_OPENMP OFF CACHE BOOL "Disable OpenMP in GGML" FORCE)
add_subdirectory(${WHISPER_DIR} whisper_build)

# Include directories
set(INCLUDES
    ${WHISPER_DIR}
    ${WHISPER_DIR}/ggml/include  # Add GGML includes
    ${WHISPER_DIR}/ggml/src      # Add GGML src folder for generated headers
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_DIR}/include
    ${WHISPER_DIR}/examples      # Sometimes helper headers are here
    ${CMAKE_BINARY_DIR}/whisper_build/ggml/src # For generated headers
)

# Add Windows system libraries needed by SDL2
set(WIN_LIBRARIES
    setupapi
    ole32
    oleaut32
    imm32
    version
    winmm
    gdi32
    cfgmgr32
    user32
    mingw32 # Add mingw32 explicitly
)

# Common libraries
set(LIBRARIES
    ${SDL2_LIBRARY}
    whisper
    ggml
    ggml-base
    ggml-cpu
    ${WIN_LIBRARIES}
)

# Console version
add_executable(TurboTalkText-console
    ${SOURCES}
)

target_include_directories(TurboTalkText-console PRIVATE
    ${INCLUDES}
)

# Console version uses -mconsole flag to show console
target_link_libraries(TurboTalkText-console PRIVATE
    mingw32 # Explicit order matters
    ${LIBRARIES}
)
target_link_options(TurboTalkText-console PRIVATE -mconsole)

# Copy SDL2.dll for the executable
if(WIN32)
    add_custom_command(TARGET TurboTalkText-console POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_DIR}/bin/SDL2.dll"
        $<TARGET_FILE_DIR:TurboTalkText-console>)
endif()

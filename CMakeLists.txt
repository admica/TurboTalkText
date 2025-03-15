cmake_minimum_required(VERSION 3.10)
project(TurboTalkText)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

add_executable(TurboTalkText src/main.cpp)
set_target_properties(TurboTalkText PROPERTIES SUFFIX ".exe")

# SDL3 manual configuration
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/SDL3-mingw)
include_directories(${SDL3_DIR}/x86_64-w64-mingw32/include/SDL3)
target_link_libraries(TurboTalkText ${SDL3_DIR}/x86_64-w64-mingw32/lib/libSDL3.dll.a)

# Whisper.cpp setup
add_library(whisper STATIC whisper.cpp/src/whisper.cpp)
target_include_directories(whisper PUBLIC whisper.cpp/include whisper.cpp/src)
target_link_libraries(TurboTalkText whisper)

# Add src/ to main executable includes for consistency
include_directories(whisper.cpp/src)

target_link_libraries(TurboTalkText user32)

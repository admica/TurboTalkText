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

set(SDL3_DIR ${CMAKE_SOURCE_DIR}/SDL3-mingw)
include_directories(${SDL3_DIR}/x86_64-w64-mingw32/include/SDL3)
target_link_libraries(TurboTalkText ${SDL3_DIR}/x86_64-w64-mingw32/lib/libSDL3.dll.a)

# Build GGML library first
add_library(ggml STATIC whisper.cpp/ggml/src/ggml.c)
target_include_directories(ggml PUBLIC whisper.cpp/ggml/include)

# Build Whisper with GGML
add_library(whisper STATIC whisper.cpp/src/whisper.cpp)
target_include_directories(whisper PUBLIC 
    whisper.cpp/include 
    whisper.cpp/ggml/include)
target_link_libraries(whisper ggml)

# Link our app with both libraries
target_link_libraries(TurboTalkText whisper ggml user32)

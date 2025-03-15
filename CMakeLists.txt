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
find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIRS})
target_link_libraries(TurboTalkText ${SDL3_LIBRARIES})

add_library(whisper STATIC whisper.cpp/src/whisper.cpp)
target_include_directories(whisper PUBLIC whisper.cpp/include)
target_link_libraries(TurboTalkText whisper)

target_link_libraries(TurboTalkText user32)

cmake_minimum_required(VERSION 3.10)
project(TurboTalkText)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For Windows using MinGW
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Add executable
add_executable(TurboTalkText src/main.cpp)
set_target_properties(TurboTalkText PROPERTIES SUFFIX ".exe")

# SDL3 setup
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/SDL3-mingw)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL3_DIR}/x86_64-w64-mingw32/include
    ${CMAKE_SOURCE_DIR}/whisper.cpp/include
)

# Add a dummy whisper implementation for now
target_compile_definitions(TurboTalkText PRIVATE -DWHISPER_DISABLED=1)

# Link libraries
target_link_libraries(TurboTalkText 
    ${SDL3_DIR}/x86_64-w64-mingw32/lib/libSDL3.dll.a
    user32
)

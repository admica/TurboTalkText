#!/bin/bash

# Exit on any error
set -e

# Color definitions for output
CYAN="\033[36m"
YELLOW="\033[33m"
RED="\033[31m"
GREEN="\033[32m"
WHITE="\033[97m"
RESET="\033[0m"

# Print functions for user feedback
print_header() { echo -e "${GREEN}[ $1 ]${RESET}"; }
print_detail() { echo -e "${CYAN}$1${RESET}"; }
print_debug() { echo -e "${YELLOW}$1${RESET}"; }
print_error() { echo -e "${RED}ERROR: $1${RESET}"; exit 1; }

# Define versions and paths
SDL3_VER="3.2.8"
WHISPER_VER="1.7.4"
PROJECT_DIR="$(pwd)"
BUILD_DIR="$PROJECT_DIR/build"
SDL3_DIR="$PROJECT_DIR/SDL3-mingw"
WHISPER_DIR="$PROJECT_DIR/whisper.cpp"
IMGUI_DIR="$PROJECT_DIR/src/imgui"
MODEL_FILE="$WHISPER_DIR/models/ggml-base.en.bin"
EXE_FILE="$BUILD_DIR/TurboTalkText.exe"
DLL_FILE="$SDL3_DIR/x86_64-w64-mingw32/bin/SDL3.dll"

# Determine output directory (user-specified or default)
if [[ -n "$1" ]]; then
    OUTPUT_DIR="$1"
    if [ ! -d "$OUTPUT_DIR" ]; then
        print_error "Output directory $OUTPUT_DIR does not exist!"
    fi
else
    FIRST_USER=$(ls /mnt/c/Users/ | awk '{print $1}' | head -n1)
    if [ -z "$FIRST_USER" ]; then
        print_error "No users found in /mnt/c/Users/!"
    fi
    OUTPUT_DIR="/mnt/c/Users/$FIRST_USER/Downloads"
fi

print_debug "Starting TurboTalkText build process"
print_debug "SDL3 version: $SDL3_VER"
print_debug "Whisper.cpp version: $WHISPER_VER"
print_debug "Output directory: $OUTPUT_DIR"

# Step 1: Install required Linux packages
print_header "Installing required Linux packages"
sudo apt update
sudo apt install -y wget cmake make mingw-w64 git unzip

# Step 2: Download and set up SDL3
if [ ! -d "$SDL3_DIR" ]; then
    print_header "Downloading SDL3"
    wget https://libsdl.org/release/SDL3-$SDL3_VER.tar.gz
    tar xf SDL3-$SDL3_VER.tar.gz
    mv SDL3-$SDL3_VER "$SDL3_DIR"
    rm SDL3-$SDL3_VER.tar.gz
fi

# Step 3: Download and set up whisper.cpp
if [ ! -d "$WHISPER_DIR" ]; then
    print_header "Cloning whisper.cpp"
    git clone https://github.com/ggerganov/whisper.cpp.git "$WHISPER_DIR"
    cd "$WHISPER_DIR"
    git checkout v$WHISPER_VER
    cd "$PROJECT_DIR"
fi

# Step 4: Download and set up spdlog
if [ ! -d "external/spdlog" ]; then
    print_header "Cloning spdlog submodule"
    git submodule add https://github.com/gabime/spdlog.git external/spdlog
    git submodule update --init --recursive
fi

# Step 5: Download and set up ImGui
if [ ! -d "$IMGUI_DIR" ]; then
    print_header "Downloading ImGui"
    mkdir -p "$IMGUI_DIR"
    wget https://github.com/ocornut/imgui/archive/refs/heads/master.zip
    unzip master.zip -d "$IMGUI_DIR"
    mv "$IMGUI_DIR/imgui-master/"* "$IMGUI_DIR/"
    rm -rf "$IMGUI_DIR/imgui-master" master.zip
fi

# Step 6: Create or clear build directory
if [ -d "$BUILD_DIR" ]; then
    print_detail "Clearing existing build directory"
    rm -rf "$BUILD_DIR"/*
else
    print_detail "Creating build directory"
    mkdir "$BUILD_DIR"
fi

# Change to build directory
cd "$BUILD_DIR"

# Step 7: Configure with CMake
print_header "Configuring with CMake"
cmake .. -G "Unix Makefiles" \
    -DSDL3_DIR="$SDL3_DIR" \
    -DCMAKE_CXX_FLAGS="-I$SDL3_DIR/x86_64-w64-mingw32/include -I/usr/x86_64-w64-mingw32/include" \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
if [ $? -ne 0 ]; then
    print_error "CMake configuration failed!"
fi

# Step 8: Compile the project
print_header "Compiling TurboTalkText"
make -j$(nproc)
if [ $? -ne 0 ]; then
    print_error "Compilation failed!"
fi

# Step 9: Copy output files to the output directory
print_header "Copying output files"
MINGW_GCC_DIR="/usr/lib/gcc/x86_64-w64-mingw32/13-posix"
MINGW_LIB_DIR="/usr/x86_64-w64-mingw32/lib"
REQUIRED_DLLS=(
    "$MINGW_GCC_DIR/libgcc_s_seh-1.dll"
    "$MINGW_GCC_DIR/libstdc++-6.dll"
    "$MINGW_LIB_DIR/libwinpthread-1.dll"
    "$MINGW_GCC_DIR/libgomp-1.dll"
)

if [ -f "$EXE_FILE" ] && [ -f "$DLL_FILE" ]; then
    sudo cp "$EXE_FILE" "$DLL_FILE" "$OUTPUT_DIR"
    if [ -f "$MODEL_FILE" ]; then
        sudo cp "$MODEL_FILE" "$OUTPUT_DIR"
    else
        print_debug "Warning: Whisper model file not found; you may need to download it separately."
    fi

    # Copy MinGW runtime DLLs
    print_detail "Copying MinGW runtime DLLs"
    for dll in "${REQUIRED_DLLS[@]}"; do
        if [ -f "$dll" ]; then
            sudo cp "$dll" "$OUTPUT_DIR"
            print_detail "Copied $(basename "$dll") to $OUTPUT_DIR"
        else
            print_debug "Warning: $dll not found"
        fi
    done

    print_detail "Files copied to $OUTPUT_DIR:"
    ls -lh "$OUTPUT_DIR/TurboTalkText.exe" "$OUTPUT_DIR/SDL3.dll"
else
    print_error "One or more output files missing!"
fi

print_header "Build completed successfully!"

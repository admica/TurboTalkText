#!/bin/bash

# Exit on any error
set -e

# Color definitions
CYAN="\033[36m"
YELLOW="\033[33m"
RED="\033[31m"
GREEN="\033[32m"
WHITE="\033[97m"
RESET="\033[0m"

# Print functions
print_header() { echo -e "${GREEN}[ $1 ]${RESET}"; }
print_detail() { echo -e "${CYAN}$1${RESET}"; }
print_debug() { echo -e "${YELLOW}$1${RESET}"; }
print_error() { echo -e "${RED}ERROR: $1${RESET}"; exit 1; }

# Versions and paths
SDL2_VER="2.32.2"
WHISPER_VER="1.7.4"
PROJECT_DIR="$(pwd)"
BUILD_DIR="$PROJECT_DIR/build"
SDL2_DIR="$PROJECT_DIR/SDL2-mingw"
WHISPER_DIR="$PROJECT_DIR/whisper.cpp"
IMGUI_DIR="$PROJECT_DIR/src/imgui"
MODEL_FILE="$WHISPER_DIR/models/ggml-base.en.bin"
EXE_FILE="$BUILD_DIR/TurboTalkText.exe"
DLL_FILE="$SDL2_DIR/x86_64-w64-mingw32/bin/SDL2.dll"

# Output directory
if [[ -n "$1" ]]; then
    OUTPUT_DIR="$1"
    [ ! -d "$OUTPUT_DIR" ] && print_error "Output directory $OUTPUT_DIR does not exist!"
else
    FIRST_USER=$(ls /mnt/c/Users/ | awk '{print $1}' | head -n1)
    [ -z "$FIRST_USER" ] && print_error "No users found in /mnt/c/Users/!"
    OUTPUT_DIR="/mnt/c/Users/$FIRST_USER/Downloads"
fi

print_debug "Starting TurboTalkText build process"
print_debug "SDL2 version: $SDL2_VER"
print_debug "Whisper.cpp version: $WHISPER_VER"
print_debug "Output directory: $OUTPUT_DIR"

# Install Linux packages
print_header "Installing required Linux packages"
sudo apt update
sudo apt install -y wget cmake make mingw-w64 git unzip

# Set up SDL2
if [ ! -d "$SDL2_DIR" ]; then
    print_header "Downloading SDL2"
    wget https://libsdl.org/release/SDL2-devel-$SDL2_VER-mingw.tar.gz -O SDL2.tar.gz
    tar xf SDL2.tar.gz -C "$PROJECT_DIR"
    mv "$PROJECT_DIR/SDL2-$SDL2_VER" "$SDL2_DIR"
    rm SDL2.tar.gz
fi

# Set up whisper.cpp
if [ ! -d "$WHISPER_DIR" ]; then
    print_header "Cloning whisper.cpp"
    git clone https://github.com/ggerganov/whisper.cpp.git "$WHISPER_DIR"
    cd "$WHISPER_DIR"
    git checkout v$WHISPER_VER
    bash ./models/download-ggml-model.sh base.en
    cd "$PROJECT_DIR"
fi

# Set up spdlog
if [ ! -d "external/spdlog" ]; then
    print_header "Cloning spdlog"
    mkdir -p external
    git clone https://github.com/gabime/spdlog.git external/spdlog
fi

# Set up ImGui
if [ ! -d "$IMGUI_DIR" ]; then
    print_header "Downloading ImGui"
    mkdir -p "$IMGUI_DIR"
    wget https://github.com/ocornut/imgui/archive/refs/heads/master.zip -O imgui.zip
    unzip imgui.zip -d "$IMGUI_DIR"
    mv "$IMGUI_DIR/imgui-master/"* "$IMGUI_DIR/"
    rm -rf "$IMGUI_DIR/imgui-master" imgui.zip
fi

# Prepare build directory
if [ -d "$BUILD_DIR" ]; then
    print_detail "Clearing existing build directory"
    rm -rf "$BUILD_DIR"/*
else
    print_detail "Creating build directory"
    mkdir "$BUILD_DIR"
fi
cd "$BUILD_DIR"

# Configure with CMake
print_header "Configuring with CMake"
cmake .. -G "Unix Makefiles" \
    -DSDL2_DIR="$SDL2_DIR" \
    -DCMAKE_CXX_FLAGS="-I$SDL2_DIR/x86_64-w64-mingw32/include -I/usr/x86_64-w64-mingw32/include" \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
[ $? -ne 0 ] && print_error "CMake configuration failed!"

# Compile
print_header "Compiling TurboTalkText"
make -j$(nproc)
[ $? -ne 0 ] && print_error "Compilation failed!"

# Copy output
print_header "Copying output files"
MINGW_GCC_DIR="/usr/lib/gcc/x86_64-w64-mingw32/13-posix"
MINGW_LIB_DIR="/usr/x86_64-w64-mingw32/lib"
REQUIRED_DLLs=(
    "$MINGW_GCC_DIR/libgcc_s_seh-1.dll"
    "$MINGW_GCC_DIR/libstdc++-6.dll"
    "$MINGW_LIB_DIR/libwinpthread-1.dll"
    "$MINGW_GCC_DIR/libgomp-1.dll"
)

if [ -f "$EXE_FILE" ] && [ -f "$DLL_FILE" ]; then
    sudo cp "$EXE_FILE" "$DLL_FILE" "$MODEL_FILE" "$OUTPUT_DIR"
    print_detail "Copying MinGW runtime DLLs"
    for dll in "${REQUIRED_DLLs[@]}"; do
        if [ -f "$dll" ]; then
            sudo cp "$dll" "$OUTPUT_DIR"
            print_detail "Copied $(basename "$dll") to $OUTPUT_DIR"
        else
            print_debug "Warning: $dll not found"
        fi
    done
    print_detail "Files copied to $OUTPUT_DIR:"
    ls -lh "$OUTPUT_DIR/TurboTalkText.exe" "$OUTPUT_DIR/SDL2.dll" "$OUTPUT_DIR/ggml-base.en.bin"
else
    print_error "One or more output files missing!"
fi

print_header "Build completed successfully!"

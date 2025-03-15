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
SDL3_VER=3.2.8
WHISPER_VER=1.7.4
PROJECT_DIR="$(pwd)"
BUILD_DIR="$PROJECT_DIR/build"
if [[ -n "$1" ]]; then
    OUTPUT_DIR=$1 # Allow user to specify output path, otherwise use the first user
else
    OUTPUT_DIR="/mnt/c/Users/$(ls /mnt/c/Users/|awk '{print $1}'|head -n1)/Desktop"
fi
SDL3_DIR="$PROJECT_DIR/SDL3-mingw"
MODEL_FILE="$PROJECT_DIR/whisper.cpp/ggml-base.en.bin"
EXE_FILE="$BUILD_DIR/TurboTalkText.exe"
DLL_FILE="$SDL3_DIR/x86_64-w64-mingw32/bin/SDL3.dll"

print_debug "Starting TurboTalkText build process"
print_debug "SDL3 version: $SDL3_VER"
print_debug "Whisper.cpp version: $WHISPER_VER"
print_debug "Output directory: $OUTPUT_DIR"

# Check and download whisper.cpp
if [ -d "whisper.cpp" ]; then
    print_detail "whisper.cpp directory already exists, skipping download and extraction"
else
    if [ ! -f v$WHISPER_VER.tar.gz ]; then
        print_header "Downloading whisper.cpp v$WHISPER_VER"
        wget https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v$WHISPER_VER.tar.gz
        [ $? -ne 0 ] && print_error "Failed to download whisper.cpp"
    else
        print_detail "whisper.cpp tarball already downloaded"
    fi

    print_header "Unpacking whisper tarball"
    tar xf v$WHISPER_VER.tar.gz
    [ $? -ne 0 ] && print_error "Failed to unpack whisper tarball"
    mv whisper.cpp-$WHISPER_VER whisper.cpp
fi

# Download Whisper model
if [ -f "$MODEL_FILE" ]; then
    print_detail "Whisper model ggml-base.en.bin already exists"
else
    print_header "Downloading Whisper base.en model"
    bash whisper.cpp/models/download-ggml-model.sh base.en
    [ $? -ne 0 ] && print_error "Failed to download Whisper model"
fi

# Create symbolic links for Whisper headers if missing
WHISPER_INCLUDE_DIR="$PROJECT_DIR/whisper.cpp/include"
if [ -f "$WHISPER_INCLUDE_DIR/ggml.h" ]; then
    print_detail "ggml.h already in $WHISPER_INCLUDE_DIR"
else
    print_header "Creating symbolic link for ggml.h"
    ln -s "$PROJECT_DIR/whisper.cpp/ggml/include/ggml.h" "$WHISPER_INCLUDE_DIR/ggml.h"
    [ $? -ne 0 ] && print_error "Failed to create ggml.h symlink"
fi

if [ -f "$WHISPER_INCLUDE_DIR/whisper.h" ]; then
    print_detail "whisper.h already in $WHISPER_INCLUDE_DIR"
else
    print_header "Creating symbolic link for whisper.h"
    ln -s "$PROJECT_DIR/whisper.cpp/include/whisper.h" "$WHISPER_INCLUDE_DIR/whisper.h"
    [ $? -ne 0 ] && print_error "Failed to create whisper.h symlink"
fi

# Check and download SDL3
if [ -d "SDL3-$SDL3_VER" ]; then
    print_detail "SDL3-$SDL3_VER directory already exists, skipping download and extraction"
else
    if [ ! -f SDL3-devel-$SDL3_VER-mingw.tar.gz ]; then
        print_header "Downloading SDL3-devel-$SDL3_VER-mingw.tar.gz"
        wget https://libsdl.org/release/SDL3-devel-$SDL3_VER-mingw.tar.gz
        [ $? -ne 0 ] && print_error "Failed to download SDL3"
    else
        print_detail "SDL3 tarball already downloaded"
    fi

    print_header "Unpacking SDL3 tarball"
    tar xf SDL3-devel-$SDL3_VER-mingw.tar.gz
    [ $? -ne 0 ] && print_error "Failed to unpack SDL3 tarball"
fi

# Check and create symbolic link for SDL3
if [ -L "SDL3-mingw" ] || [ -d "SDL3-mingw" ]; then
    print_detail "SDL3-mingw link already exists"
else
    print_header "Creating SDL3-mingw link"
    ln -s SDL3-$SDL3_VER SDL3-mingw
    [ $? -ne 0 ] && print_error "Failed to create symbolic link"
fi

# Create or clear build directory
if [ -d "$BUILD_DIR" ]; then
    print_detail "Clearing existing build directory"
    rm -rf "$BUILD_DIR"/*
else
    print_detail "Creating build directory"
    mkdir "$BUILD_DIR"
fi

# Change to build directory
cd "$BUILD_DIR"

# Run CMake to generate build files
print_header "Configuring with CMake"
cmake .. -G "Unix Makefiles" -DSDL3_DIR="$SDL3_DIR"
[ $? -ne 0 ] && print_error "CMake configuration failed!"

# Compile the project
print_header "Compiling TurboTalkText"
make
[ $? -ne 0 ] && print_error "Compilation failed!"

# Copy output files to Windows Desktop
print_header "Copying output files"
if [ -f "$EXE_FILE" ] && [ -f "$DLL_FILE" ] && [ -f "$MODEL_FILE" ]; then
    cp "$EXE_FILE" "$DLL_FILE" "$MODEL_FILE" "$OUTPUT_DIR"
    print_detail "Files copied to $OUTPUT_DIR:"
    ls -lh "$OUTPUT_DIR/TurboTalkText.exe" "$OUTPUT_DIR/SDL3.dll" "$OUTPUT_DIR/ggml-base.en.bin"
else
    print_error "One or more output files missing!"
fi

print_header "Build completed successfully!"

#!/bin/bash

# Color definitions
CYAN="\033[36m"
YELLOW="\033[33m"
RED="\033[31m"
GREEN="\033[32m"
WHITE="\033[97m"
RESET="\033[0m"

# Print functions
print_header() {
    echo -e "${GREEN}[ $1 ]${RESET}"
}

print_detail() {
    echo -e "${CYAN}$1${RESET}"
}

print_debug() {
    echo -e "${YELLOW}$1${RESET}"
}

print_error() {
    echo -e "${RED}ERROR: $1${RESET}"
    return 1
}

SDL3_VER=3.2.8
WHISPER_VER=1.7.4

print_debug "SDL3 version: $SDL3_VER"
print_debug "Whisper.cpp version: $WHISPER_VER"

# Check and download whisper.cpp
if [ -d "whisper.cpp-$WHISPER_VER" ]; then
    print_detail "whisper.cpp-$WHISPER_VER directory already exists, skipping download and extraction"
else
    if [ ! -f v$WHISPER_VER.tar.gz ]; then
        print_header "Downloading whisper.cpp v$WHISPER_VER"
        wget https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v$WHISPER_VER.tar.gz
        if [ $? -ne 0 ]; then
            print_error "Failed to download whisper.cpp"
            exit 1
        fi
    else
        print_detail "whisper.cpp tarball already downloaded"
    fi

    print_header "Unpacking whisper tarball"
    tar xf v$WHISPER_VER.tar.gz
    if [ $? -ne 0 ]; then
        print_error "Failed to unpack whisper tarball"
        exit 1
    fi
fi

# Check and download SDL3
if [ -d "SDL3-$SDL3_VER" ]; then
    print_detail "SDL3-$SDL3_VER directory already exists, skipping download and extraction"
else
    if [ ! -f SDL3-devel-$SDL3_VER-mingw.tar.gz ]; then 
        print_header "Downloading SDL3-devel-$SDL3_VER-mingw.tar.gz"
        wget https://libsdl.org/release/SDL3-devel-$SDL3_VER-mingw.tar.gz
        if [ $? -ne 0 ]; then
            print_error "Failed to download SDL3"
            exit 1
        fi
    else
        print_detail "SDL3 tarball already downloaded"
    fi

    print_header "Unpacking SDL3 tarball"
    tar xf SDL3-devel-$SDL3_VER-mingw.tar.gz
    if [ $? -ne 0 ]; then
        print_error "Failed to unpack SDL3 tarball"
        exit 1
    fi
fi

# Check and create symbolic link
if [ -L "SDL3-mingw" ] || [ -d "SDL3-mingw" ]; then
    print_detail "SDL3-mingw link already exists"
else
    print_header "Creating link"
    ln -s SDL3-$SDL3_VER SDL3-mingw
    if [ $? -ne 0 ]; then
        print_error "Failed to create symbolic link"
        exit 1
    fi
fi

print_header "Setup completed successfully!"


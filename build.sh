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

# Get MinGW compiler details
MINGW_VERSION=$(x86_64-w64-mingw32-gcc -dumpversion | cut -d. -f1)
MINGW_GCC_DIR="/usr/lib/gcc/x86_64-w64-mingw32/${MINGW_VERSION}-win32"
MINGW_LIB_DIR="/usr/x86_64-w64-mingw32/lib"

# Versions and paths
SDL2_VER="2.32.2"
WHISPER_VER="1.7.4"
PROJECT_DIR="$(pwd)"
BUILD_DIR="$PROJECT_DIR/build"
SDL2_DIR="$PROJECT_DIR/SDL2-mingw"
WHISPER_DIR="$PROJECT_DIR/whisper.cpp"
SPDLOG_DIR="$PROJECT_DIR/external/spdlog"
NLOHMANN_DIR="$PROJECT_DIR/include/nlohmann"
MODEL_FILE="$WHISPER_DIR/models/ggml-base.en.bin"
GUI_EXE_FILE="$BUILD_DIR/TurboTalkText.exe"
CONSOLE_EXE_FILE="$BUILD_DIR/TurboTalkText-console.exe"
DLL_FILE="$SDL2_DIR/x86_64-w64-mingw32/bin/SDL2.dll"

# Output directory
if [[ -n "$1" ]]; then
    OUTPUT_DIR="$1"
    [ ! -d "$OUTPUT_DIR" ] && print_error "Output directory $OUTPUT_DIR does not exist!"
else
    FIRST_USER=$(ls /mnt/c/Users/ | awk '{print $1}' | head -n1)
    [ -z "$FIRST_USER" ] && print_error "No users found in /mnt/c/Users/!"
    OUTPUT_DIR="/mnt/c/Users/$FIRST_USER/Downloads/TurboTalkText"
    # Create output directory if it doesn't exist
    mkdir -p "$OUTPUT_DIR"
fi

print_debug "Starting TurboTalkText build process"
print_debug "SDL2 version: $SDL2_VER"
print_debug "Whisper.cpp version: $WHISPER_VER"
print_debug "MinGW GCC directory: $MINGW_GCC_DIR"
print_debug "Output directory: $OUTPUT_DIR"

# Install Linux packages (include libgomp for OpenMP if needed)
print_header "Installing required Linux packages"
if ! command -v wget >/dev/null || ! command -v cmake >/dev/null || ! command -v make >/dev/null || ! command -v x86_64-w64-mingw32-gcc >/dev/null; then
    sudo apt update
    sudo apt install -y wget cmake make mingw-w64 git unzip g++-mingw-w64-x86-64 libgomp1
else
    print_detail "Required packages already installed"
fi

# Set up SDL2
if [ ! -d "$SDL2_DIR" ]; then
    print_header "Downloading SDL2"
    wget "https://libsdl.org/release/SDL2-devel-$SDL2_VER-mingw.tar.gz" -O SDL2.tar.gz
    tar xf SDL2.tar.gz -C "$PROJECT_DIR"
    mv "$PROJECT_DIR/SDL2-$SDL2_VER" "$SDL2_DIR"
    rm SDL2.tar.gz
else
    print_detail "SDL2 already present at $SDL2_DIR"
fi

# Set up whisper.cpp
if [ ! -d "$WHISPER_DIR" ]; then
    print_header "Cloning whisper.cpp"
    git clone https://github.com/ggerganov/whisper.cpp.git "$WHISPER_DIR"
    cd "$WHISPER_DIR"
    git checkout v$WHISPER_VER
    bash ./models/download-ggml-model.sh base.en
    cd "$PROJECT_DIR"
elif [ ! -f "$MODEL_FILE" ]; then
    print_header "Downloading Whisper model"
    cd "$WHISPER_DIR"
    bash ./models/download-ggml-model.sh base.en
    cd "$PROJECT_DIR"
else
    print_detail "whisper.cpp and model already present at $WHISPER_DIR"
fi

# Set up spdlog
if [ ! -d "$SPDLOG_DIR" ]; then
    print_header "Cloning spdlog"
    mkdir -p external
    git clone https://github.com/gabime/spdlog.git "$SPDLOG_DIR"
    cd "$SPDLOG_DIR"
    git checkout v1.x
    cd "$PROJECT_DIR"
else
    print_detail "spdlog already present at $SPDLOG_DIR"
fi

# Set up nlohmann/json
if [ ! -f "$NLOHMANN_DIR/json.hpp" ]; then
    print_header "Downloading nlohmann/json"
    mkdir -p "$NLOHMANN_DIR"
    wget "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" -O "$NLOHMANN_DIR/json.hpp"
else
    print_detail "nlohmann/json already present at $NLOHMANN_DIR"
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
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DBUILD_SHARED_LIBS=OFF \
    -DWHISPER_STATIC=ON \
    -DGGML_STATIC=ON
[ $? -ne 0 ] && print_error "CMake configuration failed!"

# Compile
print_header "Compiling TurboTalkText"
make -j$(nproc)
[ $? -ne 0 ] && print_error "Compilation failed!"

print_debug "Checking executable size (should be large if statically linked):"
du -h "$CONSOLE_EXE_FILE"

# Verify build with objdump to confirm static linking
print_header "Verifying build outputs"
if command -v x86_64-w64-mingw32-objdump &>/dev/null; then
    print_detail "Checking DLL dependencies (should show minimal DLLs):"
    x86_64-w64-mingw32-objdump -p "$CONSOLE_EXE_FILE" | grep -A 20 "DLL Name" || true
fi

# Copy files to output directory
print_header "Deploying to Windows"

if [ -f "$CONSOLE_EXE_FILE" ] && [ -f "$DLL_FILE" ] && [ -f "$MODEL_FILE" ]; then
    # Create a single app directory in Downloads for better organization
    APP_DIR="$OUTPUT_DIR"
    sudo mkdir -p "$APP_DIR"
    
    # Copy the executables, settings, and required files
    sudo cp "$CONSOLE_EXE_FILE" "$DLL_FILE" "$MODEL_FILE" "$APP_DIR"
    
    # Copy settings.json if it exists, otherwise create a default one
    if [ -f "settings.json" ]; then
        sudo cp "settings.json" "$APP_DIR"
        print_detail "Copied settings.json to $APP_DIR"
    else
        print_detail "Creating default settings.json in $APP_DIR"
        cat > "$APP_DIR/settings.json" << 'EOF'
{
    "audio": {
        "device": "default",
        "sample_rate": 16000,
        "silence_threshold": 0.001000,
        "silence_duration_ms": 1000
    },
    "whisper": {
        "model_path": "ggml-base.en.bin",
        "language": "en",
        "translate": false,
        "beam_size": 10,
        "threads": 4
    },
    "output": {
        "type": "keyboard",
        "add_punctuation": true,
        "capitalize_sentences": true
    }
}
EOF
    fi
    
    print_detail "Copied console executable, SDL2.dll, and Whisper model to $APP_DIR"
    
    # Make sure to get consistent architecture (x86_64) and threading model (win32)
    MINGW_SEARCH_PATHS=(
        "/usr/lib/gcc/x86_64-w64-mingw32/${MINGW_VERSION}-win32"
        "/usr/lib/gcc/x86_64-w64-mingw32/13-win32"
        "/usr/lib/gcc/x86_64-w64-mingw32/12-win32"
        "/usr/lib/gcc/x86_64-w64-mingw32/11-win32"
        "/usr/lib/gcc/x86_64-w64-mingw32/13-win32-win32"
        "/usr/lib/gcc/x86_64-w64-mingw32/12-win32-win32"
        "/usr/lib/gcc/x86_64-w64-mingw32/11-win32-win32"
        "/usr/x86_64-w64-mingw32/lib"
    )

    # Required MinGW DLLs
    REQUIRED_DLLS=(
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
    )

    # Function to find the correct DLL in search paths
    find_consistent_dll() {
        local dll_name="$1"
        for search_path in "${MINGW_SEARCH_PATHS[@]}"; do
            if [ -f "$search_path/$dll_name" ]; then
                echo "$search_path/$dll_name"
                return 0
            fi
        done
        
        # Backup option - find any x86_64 dll with win32 in the path
        find /usr -path "*x86_64*win32*/$dll_name" 2>/dev/null | head -n1
        return 0
    }

    # Copy MinGW runtime DLLs with architecture checking
    print_detail "Copying MinGW runtime DLLs:"
    for dll_name in "${REQUIRED_DLLS[@]}"; do
        dll_path=$(find_consistent_dll "$dll_name")
        
        if [ -n "$dll_path" ] && [ -f "$dll_path" ]; then
            # Verify this is a 64-bit DLL
            if file "$dll_path" | grep -q "x86-64" || file "$dll_path" | grep -q "PE32+" || echo "$dll_path" | grep -q "x86_64"; then
                sudo cp "$dll_path" "$APP_DIR"
                print_detail "✓ Copied $dll_name from $dll_path (64-bit confirmed)"
            else
                print_debug "⚠ Warning: Found $dll_path but it's not 64-bit. Searching for correct version."
                # Explicitly search for 64-bit version
                CORRECT_DLL=$(find /usr -path "*x86_64*/$dll_name" 2>/dev/null | head -n1)
                if [ -n "$CORRECT_DLL" ] && [ -f "$CORRECT_DLL" ]; then
                    sudo cp "$CORRECT_DLL" "$APP_DIR"
                    print_detail "✓ Found alternate 64-bit version of $dll_name at $CORRECT_DLL"
                else
                    print_error "Could not find 64-bit version of $dll_name"
                fi
            fi
        else
            print_error "Required DLL $dll_name not found in any MinGW directory"
        fi
    done

    print_detail "Files copied to application directory:"
    ls -la "$APP_DIR" | grep -E "\.dll|\.exe|\.bin|\.json|\.txt" | sort
    
    print_detail "Application is ready at: $APP_DIR"
    print_detail "You can run the console version using: $APP_DIR/TurboTalkText-console.exe"
else
    print_error "One or more required files missing!"
fi

print_header "Build completed successfully!"
print_header "Application deployed to: $OUTPUT_DIR"

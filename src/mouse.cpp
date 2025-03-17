#include "mouse.h"
#include "logger.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

// Helper function to normalize text for command matching (static to limit scope to this file)
static std::string normalizeText(const std::string& input) {
    // Convert to lowercase
    std::string result;
    result.resize(input.size());
    std::transform(input.begin(), input.end(), result.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
    
    // Remove punctuation
    result.erase(std::remove_if(result.begin(), result.end(), 
                 [](unsigned char c){ return std::ispunct(c); }), result.end());
                 
    return result;
}

// Helper function to extract a number from a string
static bool extractNumber(const std::string& input, int& number) {
    // Define a regex pattern to match numbers
    std::regex numberPattern("\\b(\\d+)\\b");
    std::smatch matches;
    
    if (std::regex_search(input, matches, numberPattern) && matches.size() > 1) {
        try {
            number = std::stoi(matches[1].str());
            return true;
        } catch (std::exception& e) {
            // Handle conversion errors
            return false;
        }
    }
    return false;
}

void Mouse::moveRelative(int dx, int dy) {
    // Get current mouse position
    POINT currentPos;
    if (GetCursorPos(&currentPos)) {
        // Move the mouse cursor
        SetCursorPos(currentPos.x + dx, currentPos.y + dy);
        Logger::info("Mouse moved by (" + std::to_string(dx) + ", " + std::to_string(dy) + ")");
    } else {
        Logger::error("Failed to get cursor position");
    }
}

void Mouse::getPosition(int& x, int& y) {
    POINT currentPos;
    if (GetCursorPos(&currentPos)) {
        x = currentPos.x;
        y = currentPos.y;
    } else {
        Logger::error("Failed to get cursor position");
        x = 0;
        y = 0;
    }
}

void Mouse::setMovementSpeed(int speed) {
    movementSpeed = speed;
    Logger::info("Mouse movement speed set to " + std::to_string(speed));
}

bool Mouse::processCommand(const std::string& command) {
    // Normalize the command for more flexible matching
    std::string normalizedCommand = normalizeText(command);
    
    // Simple command words to look for
    const std::string UP_COMMANDS[] = {"up", "upward", "move up", "go up"};
    const std::string DOWN_COMMANDS[] = {"down", "downward", "move down", "go down"};
    const std::string LEFT_COMMANDS[] = {"left", "move left", "go left"};
    const std::string RIGHT_COMMANDS[] = {"right", "move right", "go right"};
    const std::string FASTER_COMMANDS[] = {"faster", "speed up", "increase speed"};
    const std::string SLOWER_COMMANDS[] = {"slower", "slow down", "decrease speed"};
    
    // Try to extract a numeric value for precise movement
    int pixels = 0;
    bool hasPixelValue = extractNumber(normalizedCommand, pixels);
    
    // If no number was found, use the default movement speed
    if (!hasPixelValue) {
        pixels = movementSpeed;
    } else {
        // Limit to reasonable values to prevent accidental huge movements
        pixels = std::min(pixels, 1000);
    }
    
    // Up commands
    for (const auto& cmd : UP_COMMANDS) {
        if (normalizedCommand.find(cmd) != std::string::npos) {
            moveRelative(0, -pixels);
            return true;
        }
    }
    
    // Down commands
    for (const auto& cmd : DOWN_COMMANDS) {
        if (normalizedCommand.find(cmd) != std::string::npos) {
            moveRelative(0, pixels);
            return true;
        }
    }
    
    // Left commands
    for (const auto& cmd : LEFT_COMMANDS) {
        if (normalizedCommand.find(cmd) != std::string::npos) {
            moveRelative(-pixels, 0);
            return true;
        }
    }
    
    // Right commands
    for (const auto& cmd : RIGHT_COMMANDS) {
        if (normalizedCommand.find(cmd) != std::string::npos) {
            moveRelative(pixels, 0);
            return true;
        }
    }
    
    // Speed adjustment commands
    for (const auto& cmd : FASTER_COMMANDS) {
        if (normalizedCommand.find(cmd) != std::string::npos) {
            setMovementSpeed(movementSpeed + 10);
            return true;
        }
    }
    
    for (const auto& cmd : SLOWER_COMMANDS) {
        if (normalizedCommand.find(cmd) != std::string::npos) {
            setMovementSpeed(std::max(5, movementSpeed - 10));
            return true;
        }
    }
    
    // Click commands
    if (normalizedCommand.find("click") != std::string::npos) {
        // Simulate mouse click
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        Sleep(10);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        Logger::info("Mouse clicked");
        return true;
    }
    
    if (normalizedCommand.find("right click") != std::string::npos) {
        // Simulate right click
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        Sleep(10);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        Logger::info("Mouse right-clicked");
        return true;
    }
    
    if (normalizedCommand.find("double click") != std::string::npos) {
        // Simulate double click
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        Sleep(10);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        Sleep(100);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        Sleep(10);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        Logger::info("Mouse double-clicked");
        return true;
    }
    
    return false; // Command not recognized
} 
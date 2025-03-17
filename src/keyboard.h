#pragma once

#include <string>
#include <windows.h>
#include <vector>
#include <map>

class Keyboard {
public:
    Keyboard();
    
    // Type text character by character
    void typeText(const std::string& text);
    
    // Press a single key
    bool pressKey(const std::string& keyName);
    
    // Press a combination of keys (like Ctrl+Alt+Delete)
    bool pressKeyCombo(const std::vector<std::string>& keyNames);
    
    // Process a key command from a voice input
    bool processKeyCommand(const std::string& command);

private:
    // Initialize the key name to virtual key code mapping
    void initKeyNameMap();
    
    // Helper method to send a key press and release
    void sendKeyPress(WORD vkCode);
    
    // Parse key names from a command string
    std::vector<std::string> parseKeyNames(const std::string& command);
    
    // Map of key names to Windows virtual key codes
    std::map<std::string, WORD> keyNameMap;
};

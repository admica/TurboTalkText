#include "keyboard.h"
#include "logger.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

Keyboard::Keyboard() {
    initKeyNameMap();
}

void Keyboard::typeText(const std::string& text) {
    Logger::info("Typing text: " + text);

    for (char c : text) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = c;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        SendInput(1, &input, sizeof(INPUT));

        input.ki.dwFlags |= KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

void Keyboard::initKeyNameMap() {
    // Alphabet keys
    for (char c = 'A'; c <= 'Z'; c++) {
        std::string keyName(1, c);
        keyNameMap[keyName] = c;
        keyNameMap[std::string(1, std::tolower(c))] = c;
    }
    
    // Numeric keys
    for (char c = '0'; c <= '9'; c++) {
        std::string keyName(1, c);
        keyNameMap[keyName] = c;
    }
    
    // Function keys
    for (int i = 1; i <= 12; i++) {
        std::string keyName = "f" + std::to_string(i);
        keyNameMap[keyName] = VK_F1 + (i - 1);
    }
    
    // Special keys
    keyNameMap["enter"] = VK_RETURN;
    keyNameMap["return"] = VK_RETURN;
    keyNameMap["tab"] = VK_TAB;
    keyNameMap["space"] = VK_SPACE;
    keyNameMap["backspace"] = VK_BACK;
    keyNameMap["back"] = VK_BACK;
    keyNameMap["delete"] = VK_DELETE;
    keyNameMap["del"] = VK_DELETE;
    keyNameMap["insert"] = VK_INSERT;
    keyNameMap["ins"] = VK_INSERT;
    keyNameMap["home"] = VK_HOME;
    keyNameMap["end"] = VK_END;
    keyNameMap["pageup"] = VK_PRIOR;
    keyNameMap["pagedown"] = VK_NEXT;
    keyNameMap["escape"] = VK_ESCAPE;
    keyNameMap["esc"] = VK_ESCAPE;
    keyNameMap["capslock"] = VK_CAPITAL;
    keyNameMap["caps"] = VK_CAPITAL;
    keyNameMap["numlock"] = VK_NUMLOCK;
    keyNameMap["scrolllock"] = VK_SCROLL;
    
    // Arrow keys
    keyNameMap["up"] = VK_UP;
    keyNameMap["down"] = VK_DOWN;
    keyNameMap["left"] = VK_LEFT;
    keyNameMap["right"] = VK_RIGHT;
    
    // Modifier keys
    keyNameMap["shift"] = VK_SHIFT;
    keyNameMap["control"] = VK_CONTROL;
    keyNameMap["ctrl"] = VK_CONTROL;
    keyNameMap["alt"] = VK_MENU;
    keyNameMap["win"] = VK_LWIN;
    keyNameMap["windows"] = VK_LWIN;
    
    // Punctuation keys
    keyNameMap["period"] = VK_OEM_PERIOD;
    keyNameMap["dot"] = VK_OEM_PERIOD;
    keyNameMap["comma"] = VK_OEM_COMMA;
    keyNameMap["semicolon"] = VK_OEM_1;
    keyNameMap["colon"] = VK_OEM_1;
    keyNameMap["slash"] = VK_OEM_2;
    keyNameMap["question"] = VK_OEM_2;
    keyNameMap["tilde"] = VK_OEM_3;
    keyNameMap["backquote"] = VK_OEM_3;
    keyNameMap["bracket"] = VK_OEM_4;
    keyNameMap["backslash"] = VK_OEM_5;
    keyNameMap["closebracket"] = VK_OEM_6;
    keyNameMap["quote"] = VK_OEM_7;
    keyNameMap["minus"] = VK_OEM_MINUS;
    keyNameMap["dash"] = VK_OEM_MINUS;
    keyNameMap["plus"] = VK_OEM_PLUS;
    keyNameMap["equals"] = VK_OEM_PLUS;
}

void Keyboard::sendKeyPress(WORD vkCode) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));

    // Release the key
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

bool Keyboard::pressKey(const std::string& keyName) {
    std::string lowerKeyName = keyName;
    std::transform(lowerKeyName.begin(), lowerKeyName.end(), lowerKeyName.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
    
    Logger::info("Looking up key: '" + lowerKeyName + "'");
    
    auto it = keyNameMap.find(lowerKeyName);
    if (it != keyNameMap.end()) {
        WORD vkCode = it->second;
        Logger::info("Found key code " + std::to_string(vkCode) + " for key '" + lowerKeyName + "'");
        sendKeyPress(vkCode);
        return true;
    }
    
    // Key not found directly, try to find a similar key by removing any remaining special characters
    // or by checking for common variations of the key name
    for (const auto& pair : keyNameMap) {
        if (pair.first.find(lowerKeyName) != std::string::npos || 
            lowerKeyName.find(pair.first) != std::string::npos) {
            Logger::info("Found similar key '" + pair.first + "' for '" + lowerKeyName + "'");
            WORD vkCode = pair.second;
            sendKeyPress(vkCode);
            return true;
        }
    }
    
    Logger::error("Unknown key name: '" + keyName + "'. Available keys: ");
    std::string availableKeys;
    for (const auto& pair : keyNameMap) {
        if (!availableKeys.empty()) availableKeys += ", ";
        if (availableKeys.length() > 200) {
            availableKeys += "...";
            break;
        }
        availableKeys += pair.first;
    }
    Logger::error(availableKeys);
    
    return false;
}

bool Keyboard::pressKeyCombo(const std::vector<std::string>& keyNames) {
    if (keyNames.empty()) {
        return false;
    }
    
    std::vector<WORD> keyCodes;
    std::string comboDescription;
    
    // Collect all valid key codes and build description
    for (const auto& keyName : keyNames) {
        std::string lowerKeyName = keyName;
        std::transform(lowerKeyName.begin(), lowerKeyName.end(), lowerKeyName.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        
        auto it = keyNameMap.find(lowerKeyName);
        if (it != keyNameMap.end()) {
            keyCodes.push_back(it->second);
            if (!comboDescription.empty()) {
                comboDescription += "+";
            }
            comboDescription += keyName;
        } else {
            Logger::error("Unknown key in combo: " + keyName);
            return false;
        }
    }
    
    if (keyCodes.empty()) {
        return false;
    }
    
    // First press all keys in order
    std::vector<INPUT> inputs(keyCodes.size() * 2); // Space for press and release
    for (size_t i = 0; i < keyCodes.size(); i++) {
        inputs[i].type = INPUT_KEYBOARD;
        inputs[i].ki.wVk = keyCodes[i];
        inputs[i].ki.dwFlags = 0;
    }
    
    // Then release all keys in reverse order
    for (size_t i = 0; i < keyCodes.size(); i++) {
        inputs[keyCodes.size() + i].type = INPUT_KEYBOARD;
        inputs[keyCodes.size() + i].ki.wVk = keyCodes[keyCodes.size() - 1 - i];
        inputs[keyCodes.size() + i].ki.dwFlags = KEYEVENTF_KEYUP;
    }
    
    // Send all inputs at once for proper key combo
    UINT result = SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
    
    if (result == inputs.size()) {
        Logger::info("Pressed key combo: " + comboDescription);
        return true;
    } else {
        Logger::error("Failed to send key combo: " + comboDescription);
        return false;
    }
}

std::vector<std::string> Keyboard::parseKeyNames(const std::string& command) {
    std::vector<std::string> result;
    
    // Convert to lowercase for easier matching
    std::string lowerCommand = command;
    std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    // Remove any "jarvis press" or "jarvis push" prefixes
    std::regex prefixPattern("(jarvis\\s+(press|push|type|key)\\s+)");
    lowerCommand = std::regex_replace(lowerCommand, prefixPattern, "");
    
    // Split the remaining string by spaces or plus signs
    std::regex splitPattern("[\\s+]+");
    std::sregex_token_iterator iter(lowerCommand.begin(), lowerCommand.end(), splitPattern, -1);
    std::sregex_token_iterator end;
    
    for (; iter != end; ++iter) {
        std::string keyName = *iter;
        if (!keyName.empty()) {
            // Remove any punctuation from the key name
            keyName.erase(std::remove_if(keyName.begin(), keyName.end(), 
                          [](unsigned char c){ return std::ispunct(c); }), keyName.end());
            
            if (!keyName.empty()) {
                result.push_back(keyName);
                Logger::info("Extracted key name: '" + keyName + "'");
            }
        }
    }
    
    return result;
}

bool Keyboard::processKeyCommand(const std::string& command) {
    Logger::info("Processing key command: '" + command + "'");
    
    // Parse key names from the command
    std::vector<std::string> keyNames = parseKeyNames(command);
    
    if (keyNames.empty()) {
        Logger::error("No keys found in command: '" + command + "'");
        return false;
    }
    
    // Log all the parsed key names
    std::string keyNamesStr;
    for (const auto& key : keyNames) {
        if (!keyNamesStr.empty()) keyNamesStr += ", ";
        keyNamesStr += "'" + key + "'";
    }
    Logger::info("Parsed key names: [" + keyNamesStr + "]");
    
    // If there's only one key, press it
    if (keyNames.size() == 1) {
        bool result = pressKey(keyNames[0]);
        if (!result) {
            Logger::error("Failed to press key: '" + keyNames[0] + "'");
        }
        return result;
    }
    
    // If there are multiple keys, press them as a combo
    bool result = pressKeyCombo(keyNames);
    if (!result) {
        Logger::error("Failed to press key combo: '" + keyNamesStr + "'");
    }
    return result;
}

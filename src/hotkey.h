#ifndef HOTKEY_H
#define HOTKEY_H

#include <windows.h>
#include <atomic>

class Hotkey {
public:
    Hotkey();
    ~Hotkey();
    bool registerHotkey();
    void unregisterHotkey();
    bool isHotkeyPressed() const;
    bool isExitHotkeyPressed() const;
    void resetHotkeyPressed();
    void resetExitHotkeyPressed();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    HWND hWnd;
    std::atomic<bool> hotkeyPressed;
    std::atomic<bool> exitHotkeyPressed;
};

#endif // HOTKEY_H

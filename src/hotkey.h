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
    void resetHotkeyPressed();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    HWND hWnd;
    std::atomic<bool> hotkeyPressed;
};

#endif // HOTKEY_H

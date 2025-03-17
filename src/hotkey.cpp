#include "hotkey.h"
#include "logger.h"

#define HOTKEY_ID 1

Hotkey::Hotkey() : hWnd(NULL), hotkeyPressed(false) {}

Hotkey::~Hotkey() {
    unregisterHotkey();
}

bool Hotkey::registerHotkey() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TurboTalkTextHotkeyWindow";
    if (!RegisterClass(&wc)) {
        Logger::error("Failed to register window class");
        return false;
    }

    hWnd = CreateWindow("TurboTalkTextHotkeyWindow", NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), this);
    if (!hWnd) {
        Logger::error("Failed to create window");
        return false;
    }

    if (!RegisterHotKey(hWnd, HOTKEY_ID, MOD_CONTROL | MOD_SHIFT, 0)) {
        Logger::error("Failed to register hotkey");
        DestroyWindow(hWnd);
        return false;
    }

    return true;
}

void Hotkey::unregisterHotkey() {
    if (hWnd) {
        UnregisterHotKey(hWnd, HOTKEY_ID);
        DestroyWindow(hWnd);
        hWnd = NULL;
    }
}

bool Hotkey::isHotkeyPressed() const {
    return hotkeyPressed;
}

void Hotkey::resetHotkeyPressed() {
    hotkeyPressed = false;
}

LRESULT CALLBACK Hotkey::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_HOTKEY && wParam == HOTKEY_ID) {
        Hotkey* self = reinterpret_cast<Hotkey*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (self) {
            self->hotkeyPressed = true;
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

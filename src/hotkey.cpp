#include "hotkey.h"
#include "logger.h"

#define HOTKEY_ID 1
#define EXIT_HOTKEY_ID 2

Hotkey::Hotkey() : hWnd(NULL), hotkeyPressed(false), exitHotkeyPressed(false) {}

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

    // Process initial messages and ensure window is ready to receive hotkeys
    // This brief show and hide forces the window to process WM_CREATE properly
    ShowWindow(hWnd, SW_SHOW);
    ShowWindow(hWnd, SW_HIDE);
    
    // Process any pending messages to ensure WM_CREATE is handled
    MSG msg;
    while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (!RegisterHotKey(hWnd, HOTKEY_ID, MOD_CONTROL | MOD_SHIFT, 'A')) {
        Logger::error("Failed to register recording toggle hotkey (Ctrl+Shift+A)");
        DestroyWindow(hWnd);
        return false;
    }

    if (!RegisterHotKey(hWnd, EXIT_HOTKEY_ID, MOD_CONTROL | MOD_SHIFT, VK_CAPITAL)) {
        Logger::error("Failed to register exit hotkey (Ctrl+Shift+CapsLock)");
        UnregisterHotKey(hWnd, HOTKEY_ID);
        DestroyWindow(hWnd);
        return false;
    }

    Logger::info("Hotkeys registered: Ctrl+Shift+A to toggle recording, Ctrl+Shift+CapsLock to exit");
    return true;
}

void Hotkey::unregisterHotkey() {
    if (hWnd) {
        UnregisterHotKey(hWnd, HOTKEY_ID);
        UnregisterHotKey(hWnd, EXIT_HOTKEY_ID);
        DestroyWindow(hWnd);
        hWnd = NULL;
    }
}

bool Hotkey::isHotkeyPressed() const {
    return hotkeyPressed;
}

bool Hotkey::isExitHotkeyPressed() const {
    return exitHotkeyPressed;
}

void Hotkey::resetHotkeyPressed() {
    hotkeyPressed = false;
}

void Hotkey::resetExitHotkeyPressed() {
    exitHotkeyPressed = false;
}

LRESULT CALLBACK Hotkey::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_HOTKEY) {
        Hotkey* self = reinterpret_cast<Hotkey*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (self) {
            if (wParam == HOTKEY_ID) {
                self->hotkeyPressed = true;
                Logger::info("Recording toggle hotkey detected");
            } else if (wParam == EXIT_HOTKEY_ID) {
                self->exitHotkeyPressed = true;
                Logger::info("Exit hotkey detected");
            }
        }
        return 0;
    } else if (message == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

#include "overlay_ui.h"
#include "logger.h"
#include <windows.h>
#include <windowsx.h> // For GET_X_LPARAM and GET_Y_LPARAM
#include <stdexcept>
#include <cmath>
#include <algorithm>

// Constants
constexpr const char* WINDOW_CLASS_NAME = "TurboTalkTextOverlay";
constexpr int MIN_SIZE = 50;
constexpr int MAX_SIZE = 300;
constexpr float PI = 3.14159265358979323846f;

// Global singleton instance
static OverlayUI* g_overlayInstance = nullptr;

// Window procedure callback
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Get the overlay instance
    OverlayUI* overlay = g_overlayInstance;
    if (!overlay) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            overlay->render();
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TIMER: {
            // Animate and redraw
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            // Start dragging
            SetCapture(hwnd);
            overlay->positionWindow();
            return 0;
        }
        
        case WM_LBUTTONUP: {
            // Stop dragging
            ReleaseCapture();
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            // If dragging, move window
            if (GetCapture() == hwnd) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                
                // Get screen coordinates
                POINT pt = { x, y };
                ClientToScreen(hwnd, &pt);
                
                // Move window
                SetWindowPos(hwnd, NULL, pt.x - overlay->width / 2, pt.y - overlay->height / 2, 
                             0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            return 0;
        }
        
        case WM_DESTROY:
            overlay->hwnd = nullptr;
            return 0;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// Singleton access
OverlayUI& OverlayUI::getInstance() {
    if (!g_overlayInstance) {
        g_overlayInstance = new OverlayUI();
    }
    return *g_overlayInstance;
}

// Constructor
OverlayUI::OverlayUI() {}

// Destructor
OverlayUI::~OverlayUI() {
    shutdown();
}

// Initialize the overlay
bool OverlayUI::initialize(HINSTANCE hInst, const Settings& settings) {
    Logger::info("Initializing overlay UI");
    
    // Store instance handle
    hInstance = hInst;
    
    // Copy UI settings
    uiSettings = settings.ui;
    
    // Set initial size
    width = height = uiSettings.size;
    
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    if (status != Gdiplus::Ok) {
        Logger::error("Failed to initialize GDI+");
        return false;
    }
    
    // Create the overlay window
    if (!createOverlayWindow(hInstance)) {
        Logger::error("Failed to create overlay window");
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }
    
    // Set the visibility based on settings
    setVisible(uiSettings.enabled);
    
    // Set a timer for animation
    SetTimer(hwnd, 1, 16, NULL); // ~60 FPS
    
    Logger::info("Overlay UI initialized successfully");
    return true;
}

// Shut down the overlay
void OverlayUI::shutdown() {
    Logger::info("Shutting down overlay UI");
    
    // Stop the timer
    if (hwnd) {
        KillTimer(hwnd, 1);
    }
    
    // Destroy the window
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    
    // Unregister window class
    if (hInstance) {
        UnregisterClass(WINDOW_CLASS_NAME, hInstance);
    }
    
    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    
    // Clear singleton instance
    if (g_overlayInstance == this) {
        g_overlayInstance = nullptr;
    }
}

// Create the overlay window
bool OverlayUI::createOverlayWindow(HINSTANCE hInst) {
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    if (!RegisterClassEx(&wc)) {
        Logger::error("Failed to register window class");
        return false;
    }
    
    // Get primary monitor size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Calculate initial position (right side of screen, centered vertically)
    posX = screenWidth - width - 20;
    posY = (screenHeight - height) / 2;
    
    // Create the window
    hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        WINDOW_CLASS_NAME,
        "TurboTalkText Overlay",
        WS_POPUP,
        posX, posY, width, height,
        NULL, NULL, hInst, NULL
    );
    
    if (!hwnd) {
        Logger::error("Failed to create overlay window");
        return false;
    }
    
    // Set transparency level
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), static_cast<BYTE>(uiSettings.opacity * 255), LWA_ALPHA);
    
    return true;
}

// Update the window position
void OverlayUI::positionWindow() {
    if (!hwnd) return;
    
    // Get the cursor position
    POINT cursor;
    GetCursorPos(&cursor);
    
    // Position window under cursor for dragging
    SetWindowPos(hwnd, NULL, cursor.x - width / 2, cursor.y - height / 2, 
                 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// Update the overlay state
void OverlayUI::update(bool isListening, bool isContinuousMode, bool isMouseMode, float audioLvl) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    listening = isListening;
    continuousMode = isContinuousMode;
    mouseMode = isMouseMode;
    audioLevel = audioLvl;
    
    // If we should minimize when inactive and we're not listening
    if (uiSettings.minimizeWhenInactive && !listening && !minimized.load()) {
        setMinimized(true);
    }
    
    // If we're listening and we're minimized, restore
    if (listening && minimized.load()) {
        setMinimized(false);
    }
    
    // Trigger a redraw
    if (hwnd) {
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

// Show or hide the overlay
void OverlayUI::setVisible(bool isVisible) {
    visible.store(isVisible);
    
    if (hwnd) {
        ShowWindow(hwnd, isVisible ? SW_SHOW : SW_HIDE);
    }
}

// Check if overlay is visible
bool OverlayUI::isVisible() const {
    return visible.load();
}

// Minimize or restore
void OverlayUI::setMinimized(bool isMinimized) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    minimized.store(isMinimized);
    
    if (hwnd) {
        if (isMinimized) {
            // Animate to minimized size
            SetWindowPos(hwnd, NULL, 0, 0, MIN_SIZE, MIN_SIZE, 
                         SWP_NOMOVE | SWP_NOZORDER);
        } else {
            // Animate to full size
            SetWindowPos(hwnd, NULL, 0, 0, uiSettings.size, uiSettings.size, 
                         SWP_NOMOVE | SWP_NOZORDER);
        }
    }
}

// Check if minimized
bool OverlayUI::isMinimized() const {
    return minimized.load();
}

// Render the overlay
void OverlayUI::render() {
    if (!hwnd) return;
    
    // Get device context
    HDC hdc = GetDC(hwnd);
    if (!hdc) return;
    
    // Create compatible DC for double buffering
    HDC memDC = CreateCompatibleDC(hdc);
    if (!memDC) {
        ReleaseDC(hwnd, hdc);
        return;
    }
    
    // Get window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    // Create a bitmap for double buffering
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);
    if (!memBitmap) {
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);
        return;
    }
    
    // Select the bitmap into the memory DC
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    // Clear the bitmap
    RECT rect = {0, 0, windowWidth, windowHeight};
    FillRect(memDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    // Create GDI+ graphics object
    Gdiplus::Graphics graphics(memDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    
    // Draw components
    drawBackground(graphics);
    drawWaveform(graphics);
    drawModeIcon(graphics);
    drawStatusText(graphics);
    
    // Blit the memory DC to the window DC
    BitBlt(hdc, 0, 0, windowWidth, windowHeight, memDC, 0, 0, SRCCOPY);
    
    // Clean up
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
}

// Draw the background circle
void OverlayUI::drawBackground(Gdiplus::Graphics& g) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    // Get window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    // Calculate circle dimensions
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    int radius = std::min(windowWidth, windowHeight) / 2 - 5;
    
    // Create semi-transparent background brush
    Gdiplus::SolidBrush backgroundBrush(Gdiplus::Color(150, 30, 30, 30));
    
    // Draw background circle
    g.FillEllipse(&backgroundBrush, centerX - radius, centerY - radius, radius * 2, radius * 2);
    
    // Draw mode ring (thicker when active)
    float ringThickness = listening ? 3.0f : 1.5f;
    Gdiplus::Pen modePen(getModeColor(), ringThickness);
    g.DrawEllipse(&modePen, centerX - radius, centerY - radius, radius * 2, radius * 2);
}

// Draw the mode icon in the center
void OverlayUI::drawModeIcon(Gdiplus::Graphics& g) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    // Get window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    // Calculate icon dimensions
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    int iconSize = std::min(windowWidth, windowHeight) / 5;
    
    // Skip if minimized
    if (minimized.load() && iconSize < 10) return;
    
    // Create icon brush with current mode color
    Gdiplus::SolidBrush iconBrush(getModeColor());
    
    // Draw different icon based on mode
    if (mouseMode) {
        // Draw mouse cursor icon
        Gdiplus::Point points[] = {
            Gdiplus::Point(centerX, centerY - iconSize / 2),
            Gdiplus::Point(centerX + iconSize / 2, centerY),
            Gdiplus::Point(centerX + iconSize / 4, centerY),
            Gdiplus::Point(centerX + iconSize / 2, centerY + iconSize / 2),
            Gdiplus::Point(centerX, centerY + iconSize / 4),
            Gdiplus::Point(centerX, centerY - iconSize / 2)
        };
        g.FillPolygon(&iconBrush, points, 6);
    } else {
        // Draw keyboard icon
        int keyboardWidth = iconSize * 1.5f;
        int keyboardHeight = iconSize;
        g.FillRectangle(&iconBrush, 
                       centerX - keyboardWidth / 2, 
                       centerY - keyboardHeight / 2, 
                       keyboardWidth, keyboardHeight);
        
        // Draw "keys"
        Gdiplus::SolidBrush keysBrush(Gdiplus::Color(255, 30, 30, 30));
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                int keyWidth = keyboardWidth / 5;
                int keyHeight = keyboardHeight / 4;
                int keyX = centerX - keyboardWidth / 2 + keyWidth / 2 + col * keyWidth;
                int keyY = centerY - keyboardHeight / 2 + keyHeight / 2 + row * keyHeight;
                g.FillRectangle(&keysBrush, keyX, keyY, keyWidth - 2, keyHeight - 2);
            }
        }
    }
}

// Draw the waveform around the circle
void OverlayUI::drawWaveform(Gdiplus::Graphics& g) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    // Skip if minimized or not listening
    if (minimized.load() || !listening) return;
    
    // Get window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    // Calculate waveform path
    std::vector<Gdiplus::Point> wavePoints = generateWaveformPoints();
    
    // Create waveform pen
    Gdiplus::Color waveColor = getModeColor();
    // Make waveform brighter when active
    waveColor.SetValue(
        Gdiplus::Color::MakeARGB(
            255,
            std::min(255, waveColor.GetR() + 50),
            std::min(255, waveColor.GetG() + 50),
            std::min(255, waveColor.GetB() + 50)
        )
    );
    
    Gdiplus::Pen waveformPen(waveColor, 2.0f);
    
    // Draw the waveform
    if (wavePoints.size() > 2) {
        g.DrawClosedCurve(&waveformPen, &wavePoints[0], wavePoints.size());
    }
}

// Draw status text
void OverlayUI::drawStatusText(Gdiplus::Graphics& g) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    // Skip if minimized
    if (minimized.load()) return;
    
    // Get window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    // Calculate text position
    int centerX = windowWidth / 2;
    int bottomY = windowHeight - 20;
    
    // Create text format
    Gdiplus::FontFamily fontFamily(L"Arial");
    Gdiplus::Font font(&fontFamily, 10, Gdiplus::FontStyleRegular);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));
    
    // Determine status text
    std::wstring statusText;
    if (continuousMode) {
        statusText = L"CONTINUOUS";
    } else if (listening) {
        statusText = L"LISTENING";
    } else {
        statusText = mouseMode ? L"MOUSE MODE" : L"TEXT MODE";
    }
    
    // Measure text
    Gdiplus::RectF textRect;
    g.MeasureString(statusText.c_str(), -1, &font, Gdiplus::PointF(0, 0), &textRect);
    
    // Draw text centered
    g.DrawString(
        statusText.c_str(),
        -1,
        &font,
        Gdiplus::PointF(centerX - textRect.Width / 2, bottomY - textRect.Height / 2),
        &textBrush
    );
}

// Generate points for the waveform
std::vector<Gdiplus::Point> OverlayUI::generateWaveformPoints() {
    std::vector<Gdiplus::Point> points;
    
    // Get window size
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;
    
    // Calculate circle dimensions
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    int baseRadius = std::min(windowWidth, windowHeight) / 2 - 5;
    
    // Calculate amplitude based on audio level (clamp to prevent huge spikes)
    float amplitude = std::min(baseRadius * 0.2f, audioLevel * baseRadius * 2.0f);
    
    // Number of points around the circle
    const int numPoints = 48;
    
    // Generate points
    for (int i = 0; i < numPoints; i++) {
        // Calculate angle
        float angle = (float)i / numPoints * 2.0f * PI;
        
        // Add random variation to make it look more natural
        float noise = (float)(rand() % 100) / 100.0f - 0.5f;
        float radiusOffset = amplitude * noise;
        
        // Apply audio level to radius
        float radius = baseRadius + radiusOffset;
        
        // Calculate point position
        int x = centerX + static_cast<int>(cos(angle) * radius);
        int y = centerY + static_cast<int>(sin(angle) * radius);
        
        // Add point
        points.push_back(Gdiplus::Point(x, y));
    }
    
    return points;
}

// Get color for current mode
Gdiplus::Color OverlayUI::getModeColor() const {
    if (mouseMode) {
        // Green for mouse mode
        return Gdiplus::Color(255, 38, 166, 91);
    } else {
        // Blue for text mode
        return Gdiplus::Color(255, 41, 121, 255);
    }
} 
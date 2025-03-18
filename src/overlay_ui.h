#pragma once

#include "settings.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

// Link GDI+ library
#pragma comment(lib, "gdiplus.lib")

// Forward declaration
class OverlayUI;

// Callback function for window procedure
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class OverlayUI {
public:
    // Singleton access
    static OverlayUI& getInstance();
    
    // Delete copy constructor and assignment operator
    OverlayUI(const OverlayUI&) = delete;
    OverlayUI& operator=(const OverlayUI&) = delete;
    
    // Initialize the overlay UI
    bool initialize(HINSTANCE hInstance, const Settings& settings);
    
    // Shutdown the overlay UI
    void shutdown();
    
    // Update the overlay state
    void update(bool isListening, bool isContinuousMode, bool isMouseMode, float audioLevel);
    
    // Show or hide the overlay
    void setVisible(bool visible);
    
    // Check if the overlay is visible
    bool isVisible() const;
    
    // Minimize or restore the overlay
    void setMinimized(bool minimized);
    
    // Check if the overlay is minimized
    bool isMinimized() const;
    
    // Friend the window procedure to allow access to private members
    friend LRESULT CALLBACK ::OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Private constructor for singleton
    OverlayUI();
    ~OverlayUI();
    
    // Window creation and management
    bool createOverlayWindow(HINSTANCE hInstance);
    void positionWindow();
    
    // Drawing functions
    void render();
    void drawBackground(Gdiplus::Graphics& g);
    void drawModeIcon(Gdiplus::Graphics& g);
    void drawWaveform(Gdiplus::Graphics& g);
    void drawStatusText(Gdiplus::Graphics& g);
    
    // Generate points for the waveform
    std::vector<Gdiplus::Point> generateWaveformPoints();
    
    // Get color for current mode
    Gdiplus::Color getModeColor() const;
    
    // Window handles and state
    HWND hwnd = nullptr;
    HINSTANCE hInstance = nullptr;
    std::atomic<bool> visible = false;
    std::atomic<bool> minimized = false;
    
    // Mode information
    bool listening = false;
    bool continuousMode = false;
    bool mouseMode = false;
    float audioLevel = 0.0f;
    
    // UI settings
    Settings::UISettings uiSettings;
    
    // Position and size
    int posX = 0;
    int posY = 0;
    int width = 200;
    int height = 200;
    
    // Animation state
    float animationProgress = 0.0f;
    
    // Thread safety
    std::mutex stateMutex;
    
    // GDI+ token
    ULONG_PTR gdiplusToken = 0;
}; 
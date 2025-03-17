#ifndef MOUSE_H
#define MOUSE_H

#include <windows.h>
#include <string>

class Mouse {
public:
    // Move the mouse cursor by a relative amount
    void moveRelative(int dx, int dy);
    
    // Get current mouse position
    void getPosition(int& x, int& y);
    
    // Set the movement speed (pixels per command)
    void setMovementSpeed(int speed);
    
    // Process a command and move the mouse accordingly
    bool processCommand(const std::string& command);
    
private:
    int movementSpeed = 20; // Default movement speed in pixels
};

#endif // MOUSE_H 
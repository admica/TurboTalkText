#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <cstdio>

class Logger {
public:
    enum Level {
        ERROR,
        INFO,
        DEBUG
    };

    static void init();
    
    template<typename... Args>
    static void info(const char* fmt, const Args&... args) {
        log(INFO, fmt, args...);
    }
    
    template<typename... Args>
    static void error(const char* fmt, const Args&... args) {
        log(ERROR, fmt, args...);
    }

private:
    static Level current_level;

    template<typename... Args>
    static void log(Level level, const char* fmt, const Args&... args) {
        if (level > current_level) return;
        
        // Print level prefix
        switch (level) {
            case ERROR: std::cerr << "[ERROR] "; break;
            case INFO: std::cout << "[INFO] "; break;
            case DEBUG: std::cout << "[DEBUG] "; break;
        }
        
        // Simple printf-style formatting
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), fmt, args...);
        
        // Output based on level
        if (level == ERROR) {
            std::cerr << buffer << std::endl;
        } else {
            std::cout << buffer << std::endl;
        }
    }
};

#endif // LOGGER_H

#include "logger.h"

// Initialize static member
Logger::Level Logger::current_level = Logger::INFO;

void Logger::init() {
    // Nothing to initialize - using simple cout/cerr logging
}

#include "Application.h"
#include <iostream>

int main() {
    std::cout << "Starting Galaxy CPU Application..." << std::endl;
    
    // Create application instance
    Application app;
    
    // Initialize the application
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }
    
    // Run the main loop
    app.run();
    
    // Cleanup is handled automatically by the destructor
    std::cout << "Application finished successfully." << std::endl;
    return 0;
}

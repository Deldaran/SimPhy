cmake_minimum_required(VERSION 3.16)
project(GalaxyApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(include)

# Find required packages
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Collect source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(GalaxyApp ${SOURCES})

# Link libraries
target_link_libraries(GalaxyApp 
    PRIVATE 
    glfw 
    OpenGL::GL 
    GLEW::GLEW
    glm::glm
    imgui::imgui
)

# Set output directory
set_target_properties(GalaxyApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

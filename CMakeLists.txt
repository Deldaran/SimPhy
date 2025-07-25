cmake_minimum_required(VERSION 3.16)
project(GalaxyApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(include)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)



# Collect source files
file(GLOB SOURCES "src/*.cpp")
# Ajoute explicitement stb_image_write.cpp et tiny_obj_loader.cc si jamais ils ne sont pas pris par le GLOB
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image_write.cpp)
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_obj_loader.cc)

# Create executable
add_executable(GalaxyApp ${SOURCES})

# Link libraries
target_link_libraries(GalaxyApp 
    PRIVATE 
    glfw 
    OpenGL::GL 
    glm::glm
    imgui::imgui
    glad::glad
)

# Set output directory
set_target_properties(GalaxyApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

cmake_minimum_required(VERSION 3.16)
project(GalaxyApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Configuration des options ---
option(BUILD_USE_VCPKG "Use vcpkg for dependencies" ON)

# --- Finding Packages ---
# Note: Sur Mac avec vcpkg, c'est comme sur Windows.
# Si l'utilisateur n'utilise pas vcpkg sur Mac, il faudra ajuster.
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# --- Sources ---
file(GLOB SOURCES "src/*.cpp")
# Include other necessary files if not covered by GLOB
# list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image_write.cpp) 

# --- Executable ---
add_executable(GalaxyApp ${SOURCES})

# --- Include Directories ---
target_include_directories(GalaxyApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Linking ---
target_link_libraries(GalaxyApp 
    PRIVATE 
    glfw 
    OpenGL::GL 
    glm::glm
    imgui::imgui
    glad::glad
)

# --- Mac Specific ---
if (APPLE)
    # Sur macOS, parfois nécessaire pour le linkage statique ou frameworks
    target_link_libraries(GalaxyApp PRIVATE "-framework IOKit" "-framework Cocoa" "-framework CoreVideo")
endif()

# Output Directory
set_target_properties(GalaxyApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

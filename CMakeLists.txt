cmake_minimum_required(VERSION 3.16)
project(GalaxyApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dépendances (vcpkg) ---
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# --- Sources ---
file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB SHADERS CONFIGURE_DEPENDS "shaders/*")

add_executable(GalaxyApp ${SOURCES} ${SHADERS})
target_include_directories(GalaxyApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Les shaders sont lus au lancement depuis le dossier source :
# on peut les modifier sans recompiler, il suffit de relancer.
target_compile_definitions(GalaxyApp PRIVATE SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/")

# Les sources contiennent des accents (textes de l'interface) encodés en UTF-8.
if (MSVC)
    target_compile_options(GalaxyApp PRIVATE /utf-8)
endif()

target_link_libraries(GalaxyApp
    PRIVATE
    glfw
    OpenGL::GL
    glm::glm
    imgui::imgui
    glad::glad
)

set_target_properties(GalaxyApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

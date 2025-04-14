cmake_minimum_required(VERSION 3.17)
project(OpenGLCube)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Use FetchContent to download and build dependencies
include(FetchContent)

# GLFW
message(STATUS "Downloading and configuring GLFW...")
FetchContent_Declare(
        glfw # Target name will be 'glfw'
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM
message(STATUS "Downloading and configuring GLM...")
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm) # Header-only

# GLEW - properly download and configure
message(STATUS "Downloading and configuring GLEW...")
FetchContent_Declare(
        glew # Base name is 'glew'
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
        GIT_TAG glew-cmake-2.2.0
)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE) # Force static build
FetchContent_MakeAvailable(glew) # Should create target 'libglew_static'

# ImGui
message(STATUS "Downloading and configuring ImGui...")
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.4
)
FetchContent_MakeAvailable(imgui)

# --- Bullet Physics ---
message(STATUS "Downloading and configuring Bullet Physics...")
FetchContent_Declare(
        bullet3
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG 3.25
)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
# set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bullet3)
# --- End Bullet Physics ---


# Create ImGui library (static)
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(imgui PRIVATE glfw) # Link simple glfw target
target_include_directories(imgui PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include
)


# --- Main Executable ---
add_executable(OpenGLCube "")

# Source files
file(GLOB SOURCES "src/*.cpp")
target_sources(OpenGLCube PRIVATE ${SOURCES})

# Include directories for OpenGLCube
target_include_directories(OpenGLCube PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${glm_SOURCE_DIR}
        ${glew_SOURCE_DIR}/include      # GLEW headers
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include      # GLFW headers

        # Bullet Includes
        ${bullet3_SOURCE_DIR}
        ${bullet3_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(OpenGLCube PRIVATE
        OpenGL::GL          # Modern OpenGL target name
        glfw                # Simple GLFW target name
        libglew_static      # << CHANGED from glew::glew_static
        imgui               # Our own ImGui library

        # Bullet libraries
        BulletDynamics
        BulletCollision
        LinearMath
)

# --- Asset Copying ---
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/textures/cube_textures)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/textures/skybox)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

file(GLOB_RECURSE TEXTURE_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/textures/*.jpg" # Add other extensions if needed
        "${CMAKE_SOURCE_DIR}/textures/*.png"
)
foreach(TEXTURE_FILE ${TEXTURE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR} ${TEXTURE_FILE})
    set(DEST_FILE "${CMAKE_BINARY_DIR}/${REL_PATH}")
    get_filename_component(DEST_DIR ${DEST_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${DEST_DIR})
    configure_file(${TEXTURE_FILE} ${DEST_FILE} COPYONLY)
endforeach()

file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/shaders/*")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(FILENAME ${SHADER_FILE} NAME)
    set(DEST_FILE "${CMAKE_BINARY_DIR}/shaders/${FILENAME}")
    configure_file(${SHADER_FILE} ${DEST_FILE} COPYONLY)
endforeach()
# --- End Asset Copying ---
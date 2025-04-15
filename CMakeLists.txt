cmake_minimum_required(VERSION 3.17)
project(OpenGLCube)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Ninja as the build tool
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "")

find_package(OpenGL REQUIRED)

include(FetchContent)

# GLFW
message(STATUS "Downloading and configuring GLFW...")
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
if(TARGET glfw)
    message(STATUS "GLFW configured successfully.")
else()
    message(WARNING "GLFW target 'glfw' not found.")
endif()

# GLM
message(STATUS "Downloading and configuring GLM...")
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)
if(DEFINED glm_SOURCE_DIR)
    message(STATUS "GLM configured successfully (header only).")
else()
    message(WARNING "GLM source directory not defined.")
endif()

# GLEW
cmake_policy(SET CMP0048 NEW)
message(STATUS "Downloading and configuring GLEW...")
FetchContent_Declare(glew GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git GIT_TAG glew-cmake-2.2.0)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glew)
if(TARGET libglew_static)
    message(STATUS "GLEW configured successfully (target 'libglew_static').")
else()
    message(WARNING "GLEW target 'libglew_static' not found.")
endif()

# ImGui
message(STATUS "Downloading and configuring ImGui...")
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_MakeAvailable(imgui)
if(DEFINED imgui_SOURCE_DIR)
    message(STATUS "ImGui source configured successfully.")
else()
    message(WARNING "ImGui source directory not defined.")
endif()

# Bullet Physics
cmake_policy(SET CMP0048 NEW)
message(STATUS "Downloading and configuring Bullet Physics...")
FetchContent_Declare(bullet3 GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git GIT_TAG 3.25)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bullet3)
if(TARGET BulletDynamics AND TARGET BulletCollision AND TARGET LinearMath)
    message(STATUS "Bullet Physics configured successfully.")
else()
    message(WARNING "One or more Bullet Physics targets not found.")
endif()

# Assimp
message(STATUS "Downloading and configuring Assimp...")
FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG master
)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
if(TARGET assimp)
    message(STATUS "Assimp configured successfully.")
else()
    message(WARNING "Assimp target 'assimp' not found.")
endif()

# --- Tiny File Dialogs ---
message(STATUS "Downloading and configuring tinyfiledialogs...")
FetchContent_Declare(
        tinyfiledialogs
        GIT_REPOSITORY https://git.code.sf.net/p/tinyfiledialogs/code
        GIT_TAG master
)
FetchContent_MakeAvailable(tinyfiledialogs)
message(STATUS "Tiny File Dialogs configured.")
# --- End Tiny File Dialogs ---

# Create ImGui library (static)
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(imgui PRIVATE glfw)
target_include_directories(imgui PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${glfw_SOURCE_DIR}/include)
message(STATUS "Custom ImGui static library target created.")

add_executable(OpenGLCube "")
file(GLOB SOURCES "src/*.cpp")
target_sources(OpenGLCube PRIVATE ${SOURCES} ${tinyfiledialogs_SOURCE_DIR}/tinyfiledialogs.c)
message(STATUS "Found source files: ${SOURCES}")

target_include_directories(OpenGLCube PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${glm_SOURCE_DIR}
        ${glew_SOURCE_DIR}/include
        ${glfw_SOURCE_DIR}/include
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${bullet3_SOURCE_DIR}
        ${bullet3_SOURCE_DIR}/src
        ${assimp_SOURCE_DIR}/include
        ${tinyfiledialogs_SOURCE_DIR}
)
message(STATUS "Include directories set for OpenGLCube.")

target_link_libraries(OpenGLCube PRIVATE
        OpenGL::GL glfw libglew_static imgui
        BulletDynamics BulletCollision LinearMath
        assimp
)
if(WIN32)
    target_link_libraries(OpenGLCube PRIVATE Ole32 Shell32 User32 Comdlg32)
    message(STATUS "Linking Windows libraries for tinyfiledialogs.")
endif()
message(STATUS "Libraries linked for OpenGLCube.")

message(STATUS "Setting up asset copying...")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/textures/cube_textures)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/textures/skybox)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

file(GLOB_RECURSE TEXTURE_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/textures/*.jpg"
        "${CMAKE_SOURCE_DIR}/textures/*.png"
        "${CMAKE_SOURCE_DIR}/textures/*.jpeg"
        "${CMAKE_SOURCE_DIR}/textures/*.bmp"
        "${CMAKE_SOURCE_DIR}/textures/*.tga"
)
foreach(TEXTURE_FILE ${TEXTURE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR} ${TEXTURE_FILE})
    set(DEST_FILE "${CMAKE_BINARY_DIR}/${REL_PATH}")
    get_filename_component(DEST_DIR ${DEST_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${DEST_DIR})
    configure_file(${TEXTURE_FILE} ${DEST_FILE} COPYONLY)
endforeach()
message(STATUS "Texture copying configured.")

file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/shaders/*")
foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(FILENAME ${SHADER_FILE} NAME)
    set(DEST_FILE "${CMAKE_BINARY_DIR}/shaders/${FILENAME}")
    configure_file(${SHADER_FILE} ${DEST_FILE} COPYONLY)
endforeach()
message(STATUS "Shader copying configured.")

message(STATUS "CMake configuration finished.")
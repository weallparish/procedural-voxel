cmake_minimum_required(VERSION 3.20)
project(VoxelGame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(FetchContent)

# ---- GLFW ----
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4)

# ---- GLM ----
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1)

# ---- Dear ImGui (no CMake of its own, so we build it below) ----
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.5)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master)

FetchContent_Declare(fnl
    GIT_REPOSITORY https://github.com/auburn/fastnoiselite.git
    GIT_TAG        master)


FetchContent_MakeAvailable(glfw glm imgui stb fnl)

# ---- glad (generated files live in external/glad, see README steps) ----
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# ---- ImGui static library with GLFW + OpenGL3 backends ----
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# ---- Game ----
add_executable(voxel src/main.cpp
  src/world/Mesher.h
  src/render/Texture.h
  src/core/Input.h
  src/core/Camera.h
  src/core/Debug.h
  src/world/World.h
  src/world/TerrainGen.h
  src/render/Cull.h
  src/world/ChunkStreamer.h)
target_link_libraries(voxel PRIVATE glad glfw glm::glm imgui)

target_include_directories(voxel PRIVATE ${stb_SOURCE_DIR})
target_include_directories(voxel PRIVATE ${fnl_SOURCE_DIR})

if(MSVC)
  target_compile_options(voxel PRIVATE /W4)
else()
  target_compile_options(voxel PRIVATE -Wall -Wextra)
endif()
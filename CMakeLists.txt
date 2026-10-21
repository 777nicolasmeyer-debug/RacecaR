cmake_minimum_required(VERSION 3.20)
project(RacecaR)

set(CMAKE_CXX_STANDARD 23)

# Vcpkg dependencies (disabled; the project uses the copies in Engine/external).
# find_package(glad CONFIG REQUIRED)
# find_package(glfw3 CONFIG REQUIRED)
# find_package(Stb REQUIRED)
# find_package(Bullet CONFIG REQUIRED)
# find_package(SFML COMPONENTS Audio CONFIG REQUIRED)
# find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h
#         PATHS ${VCPKG_ROOT}/installed/x64-mingw-dynamic/include
# )

set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine/external")

set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory("${EXTERNAL_DIR}/glfw-3.5.1" "${CMAKE_CURRENT_BINARY_DIR}/external/glfw")
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
add_subdirectory("${EXTERNAL_DIR}/bullet3-3.25" "${CMAKE_CURRENT_BINARY_DIR}/external/bullet")
unset(CMAKE_POLICY_VERSION_MINIMUM)

add_library(glad "${EXTERNAL_DIR}/glad/src/glad.c")
target_include_directories(glad PUBLIC "${EXTERNAL_DIR}/glad/include")

find_package(SFML 3 COMPONENTS Audio CONFIG REQUIRED
        PATHS "${EXTERNAL_DIR}/SFML/SFML-3.1.0/lib/cmake/SFML"
        NO_DEFAULT_PATH
)

set(GLM_INCLUDE_DIR "${EXTERNAL_DIR}/glm-master/glm-master")
set(STB_INCLUDE_DIR "${EXTERNAL_DIR}")
set(TINYGLTF_INCLUDE_DIR "${EXTERNAL_DIR}/tinygltf-3.0.1/attic")

add_executable(RacecaR
        src/main.cpp
        Engine/loaders_implementation.cpp
        Engine/Car/Car.cpp
        Engine/engine.cpp
        Engine/engine_assets.cpp
        Engine/engine_player.cpp
        Engine/engine_scene.cpp
        Engine/engine_shadows.cpp
        Engine/Meshes/Meshes.cpp
        Engine/Rendering/VAO_VBO.cpp
        Engine/Shader/Shader.cpp
        Engine/Loaders/Loaders.cpp
        Engine/Math/matrix.cpp
        Engine/Camera/camera.cpp
        Engine/Math/Collisions.cpp
        Engine/Scene/SceneManager.cpp
)

target_link_libraries(RacecaR PRIVATE
        glfw
        SFML::Audio
        glad
        BulletDynamics
        BulletCollision
        LinearMath
)

target_include_directories(RacecaR PRIVATE
        ${GLM_INCLUDE_DIR}
        ${STB_INCLUDE_DIR}
        ${TINYGLTF_INCLUDE_DIR}
        "${EXTERNAL_DIR}/bullet3-3.25/src"
        Engine
)

target_compile_definitions(RacecaR PRIVATE
        ASSET_RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
        SHADER_RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Engine/Shader/"
)

#ifndef RACECAR_ENGINE_H
#define RACECAR_ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Car/Car.h"
#include "Camera/camera.h"
#include "Loaders/Loaders.h"
#include "Math/Collisions.h"
#include "Meshes/Meshes.h"
#include "Scene/SceneManager.h"
#include "Shader/Shader.h"
#include "SoundManager/SoundManager.h"

class Engine {
public:
    struct CameraInput {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
        bool shiftUp = false;
        bool shiftDown = false;
        bool brake = false;
    };

    Engine() = default;
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Call with an active OpenGL context and initialized GLAD.
    bool initialize();
    void update(float deltaTime);
    void render(int width, int height);

    void handleKeyboardInput(const CameraInput& input, float deltaTime);
    void handleMouseInput(double xpos, double ypos);
    void togglePlayer();

    void addStaticObject(const MeshData& mesh, const glm::vec3& position,
                         const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                         const glm::vec3& scale = glm::vec3(1.0f));
    void addStaticModel(const char* path, const glm::vec3& position,
                        const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                        const glm::vec3& scale = glm::vec3(1.0f));
    void addStaticModel(const char* renderPath, const char* collisionPath,
                        const glm::vec3& position,
                        const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                        const glm::vec3& scale = glm::vec3(1.0f));
    void addDynamicObject(const MeshData& mesh, const glm::vec3& position,
                          const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                          const glm::vec3& scale = glm::vec3(1.0f));
    void addDynamicModel(const std::vector<MeshData>& meshes, const glm::vec3& position,
                         const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                         const glm::vec3& scale = glm::vec3(1.0f));
    void addDynamicModel(const char* path, const glm::vec3& position,
                         const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                         const glm::vec3& scale = glm::vec3(1.0f));
    void addLight(const MeshData& markerMesh, const glm::vec3& position,
                  const glm::vec3& color = glm::vec3(1.0f),
                  bool castsShadows = true, bool persistent = false);
    void addLight(const char* modelPath, const glm::vec3& position,
                  const glm::vec3& color = glm::vec3(1.0f),
                  bool castsShadows = true, bool persistent = false);
    void clearDynamicObjects();
    void clearLights();
    glm::vec3 getCameraPosition() const;
    float getPlayerRPM() const;
    float getPlayerSpeedKmh() const;
    int getPlayerGear() const;
    bool hasPlayer() const;

private:
    static constexpr unsigned int SHADOW_MAP_SIZE = 1024;
    static constexpr float SHADOW_FAR_PLANE = 200.0f;
    static constexpr float CAMERA_FAR_PLANE = 10000.0f;

    bool initialized = false;
    bool shadowMapReady = false;
    bool shiftUpWasPressed = false;
    bool shiftDownWasPressed = false;
    float cameraSpeed = 500.0f;
    GLuint shadowFramebuffer = 0;

    Shader shader;
    Shader shadowShader;
    Camera camera;
    Collisions collisions;
    Loaders loader;
    SoundManager soundManager;
    std::unordered_map<std::string, std::vector<MeshData>> modelCache;
    std::vector<SceneObject> sceneObjects;
    std::vector<SceneObject> terrainObjects;
    std::vector<LightingObject> lights;
    Car car;

    bool initializeShadowMap();
    GLuint createShadowCubemap();
    const std::vector<MeshData>& loadModelMeshes(const char* path);
    std::unordered_map<std::string, ModelAnimationData> animationCache;
    void renderShadowMap(GLuint shadowCubemap, const glm::vec3& lightPosition,
                         int width, int height);
    void updatePhysicsTransforms();
};



#endif

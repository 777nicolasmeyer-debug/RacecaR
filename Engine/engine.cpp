#include "Engine.h"
#include <algorithm>
#include <iostream>

Engine::~Engine() {
    if (!initialized) {
        return;
    }
    car.despawn(collisions);
    clearDynamicObjects();
    for (const auto& light : lights) {
        if (light.shadowCubemap != 0) {
            glDeleteTextures(1, &light.shadowCubemap);
        }
    }
    if (shadowFramebuffer != 0) {
        glDeleteFramebuffers(1, &shadowFramebuffer);
    }
}

bool Engine::initialize() {
    if (initialized) {
        return true;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    loader.initialize();
    collisions.init();
    shader.createShaders(SHADER_RESOURCES_PATH "vertex.glsl",
                         SHADER_RESOURCES_PATH "fragment.glsl");
    shader.use();
    shader.loadTexture("tex", 0);
    shadowMapReady = initializeShadowMap();
    if (!shadowMapReady) {
        std::cerr << "Point-light shadows are disabled" << std::endl;
    }
    shader.loadTexture("shadowCube", 1);
    initialized = true;
    return true;
}

void Engine::update(float deltaTime) {
    if (!initialized) {
        return;
    }
    collisions.update(deltaTime);
    updatePhysicsTransforms();
    car.updateCamera(camera);
}

void Engine::render(const int width, const int height) {
    if (!initialized || width <= 0 || height <= 0) {
        return;
    }

    glViewport(0, 0, width, height);
    glDepthMask(GL_TRUE);
    if (shadowMapReady) {
        for (const auto& light : lights) {
            if (light.shadowCubemap != 0) {
                renderShadowMap(light.shadowCubemap, light.position, width, height);
            }
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.use();
    const glm::mat4 projection =
        glm::perspective(glm::radians(60.0f), static_cast<float>(width) / height, 0.01f,
                         CAMERA_FAR_PLANE);
    shader.loadMatrix("projection", projection);
    shader.loadMatrix("view", camera.getViewMatrix());
    shader.loadVector3("viewPos", camera.getPosition());
    glUniform1f(glGetUniformLocation(shader.ID, "farPlane"), SHADOW_FAR_PLANE);

    const auto drawObjects = [this](const bool transparent) {
        std::vector<SceneObject*> objects;
        const auto appendObjects = [&objects, transparent](std::vector<SceneObject>& source) {
            for (auto& object : source) {
                if (object.transparent == transparent) {
                    objects.push_back(&object);
                }
            }
        };
        appendObjects(sceneObjects);
        appendObjects(terrainObjects);
        for (auto& carObject : car.getRenderObjects()) {
            if (car.isActive() && carObject.transparent == transparent) {
                objects.push_back(&carObject);
            }
        }
        if (transparent) {
            const glm::vec3 cameraPosition = camera.getPosition();
            std::sort(objects.begin(), objects.end(), [&cameraPosition](const SceneObject* left,
                                                                       const SceneObject* right) {
                const glm::vec3 leftCenter =
                    left->position + left->rotation * (left->localCenter * left->scale);
                const glm::vec3 rightCenter =
                    right->position + right->rotation * (right->localCenter * right->scale);
                return glm::dot(leftCenter - cameraPosition, leftCenter - cameraPosition) >
                       glm::dot(rightCenter - cameraPosition, rightCenter - cameraPosition);
            });
        }
        for (SceneObject* object : objects) {
            object->draw(shader);
        }
    };

    if (lights.empty()) {
        shader.loadVector3("lightColor", glm::vec3(0.0f));
        shader.loadVector3("lightPos", glm::vec3(0.0f));
        glUniform1i(glGetUniformLocation(shader.ID, "shadowsEnabled"), 0);
        glDisable(GL_BLEND);
        drawObjects(false);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        drawObjects(true);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        return;
    }

    for (size_t i = 0; i < lights.size(); ++i) {
        const auto& light = lights[i];
        shader.loadVector3("lightColor", light.color);
        shader.loadVector3("lightPos", light.position);
        glUniform1i(glGetUniformLocation(shader.ID, "shadowsEnabled"),
                    light.shadowCubemap != 0 ? 1 : 0);
        glUniform1i(glGetUniformLocation(shader.ID, "ambientEnabled"), i == 0 ? 1 : 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, light.shadowCubemap);
        if (i == 0) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
        }
        glDepthFunc(i == 0 ? GL_LESS : GL_LEQUAL);
        glDepthMask(GL_TRUE);

        if (i == 0) {
            for (auto& marker : lights) marker.draw(shader);
        }
        drawObjects(false);

        glEnable(GL_BLEND);
        if (i == 0) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
        }
        glDepthMask(GL_FALSE);
        drawObjects(true);
        glDepthMask(GL_TRUE);
    }
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

#include "Engine.h"

#include <iostream>
#include <limits>
#include <unordered_set>

namespace {

SceneObject createSceneObject(const MeshData& mesh, const glm::vec3& position,
                              const glm::quat& rotation, const glm::vec3& scale) {
    SceneObject object;
    object.buffers.init(mesh);
    object.position = position;
    object.rotation = rotation;
    object.scale = scale;
    object.textureID = mesh.textureID;
    object.baseColor = mesh.baseColor;
    object.localCenter = mesh.localCenter;
    object.transparent = mesh.transparent;
    object.alphaMasked = mesh.alphaMasked;
    object.alphaCutoff = mesh.alphaCutoff;
    return object;
}

}

void Engine::addStaticObject(const MeshData& mesh, const glm::vec3& position,
                             const glm::quat& rotation, const glm::vec3& scale) {
    SceneObject object = createSceneObject(mesh, position, rotation, scale);
    collisions.triangleShapeS(object, mesh);
    terrainObjects.push_back(object);
}

void Engine::addStaticModel(const char* renderPath, const char* collisionPath,
                            const glm::vec3& position, const glm::quat& rotation,
                            const glm::vec3& scale) {
                            for (const auto& mesh : loadModelMeshes(renderPath)) {
                                terrainObjects.push_back(createSceneObject(mesh, position, rotation, scale));
    }

    size_t collisionMeshCount = 0;
    for (const auto& sourceMesh : loadModelMeshes(collisionPath)) {
        if (sourceMesh.vertices.empty() || sourceMesh.indices.size() < 3) {
            continue;
        }

        MeshData collisionMesh = sourceMesh;
        for (size_t i = 0; i + 2 < collisionMesh.vertices.size(); i += 8) {
            collisionMesh.vertices[i] *= scale.x;
            collisionMesh.vertices[i + 1] *= scale.y;
            collisionMesh.vertices[i + 2] *= scale.z;
        }

        SceneObject collisionObject;
        collisionObject.position = position;
        collisionObject.rotation = rotation;
        collisions.triangleShapeS(collisionObject, collisionMesh);
        ++collisionMeshCount;
    }
    if (collisionMeshCount == 0) {
        std::cerr << "Static collision model contains no triangle meshes: "
                  << collisionPath << std::endl;
    }
}

void Engine::addDynamicObject(const MeshData& mesh, const glm::vec3& position,
                              const glm::quat& rotation, const glm::vec3& scale) {
    if (mesh.vertices.empty()) {
        return;
    }

    SceneObject object = createSceneObject(mesh, position, rotation, scale);
    collisions.convexShapeD(object, mesh, scale);
    sceneObjects.push_back(object);
}

void Engine::addDynamicModel(const std::vector<MeshData>& meshes, const glm::vec3& position,
                             const glm::quat& rotation, const glm::vec3& scale) {
    if (meshes.empty()) {
        return;
    }

    if (meshes.size() == 1) {
        addDynamicObject(meshes.front(), position, rotation, scale);
        return;
    }

    MeshData collisionMesh;
    size_t totalVertexCount = 0;
    for (const auto& mesh : meshes) {
        totalVertexCount += mesh.vertices.size();
    }
    collisionMesh.vertices.reserve(totalVertexCount);

    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());
    for (const auto& mesh : meshes) {
        for (size_t i = 0; i + 2 < mesh.vertices.size(); i += 8) {
            const glm::vec3 point(mesh.vertices[i], mesh.vertices[i + 1], mesh.vertices[i + 2]);
            minimum = glm::min(minimum, point);
            maximum = glm::max(maximum, point);
        }
    }
    const glm::vec3 modelCenter = (minimum + maximum) * 0.5f;

    for (const auto& mesh : meshes) {
        collisionMesh.vertices.insert(collisionMesh.vertices.end(),
                                      mesh.vertices.begin(), mesh.vertices.end());
    }
    for (size_t i = 0; i + 2 < collisionMesh.vertices.size(); i += 8) {
        collisionMesh.vertices[i] -= modelCenter.x;
        collisionMesh.vertices[i + 1] -= modelCenter.y;
        collisionMesh.vertices[i + 2] -= modelCenter.z;
    }

    SceneObject physicsObject;
    physicsObject.position = position;
    physicsObject.rotation = rotation;
    collisions.boxShapeD(physicsObject, collisionMesh, scale);

    for (const auto& mesh : meshes) {
        if (mesh.vertices.empty()) {
            continue;
        }
        MeshData centeredMesh = mesh;
        for (size_t i = 0; i + 2 < centeredMesh.vertices.size(); i += 8) {
            centeredMesh.vertices[i] -= modelCenter.x;
            centeredMesh.vertices[i + 1] -= modelCenter.y;
            centeredMesh.vertices[i + 2] -= modelCenter.z;
        }
        centeredMesh.localCenter -= modelCenter;
        SceneObject object = createSceneObject(centeredMesh, position, rotation, scale);
        object.body = physicsObject.body;
        sceneObjects.push_back(object);
    }
}

void Engine::addDynamicModel(const char* path, const glm::vec3& position,
                             const glm::quat& rotation, const glm::vec3& scale) {
    addDynamicModel(loadModelMeshes(path), position, rotation, scale);
}

void Engine::addLight(const MeshData& markerMesh, const glm::vec3& position,
                      const glm::vec3& color, const bool castsShadows,
                      const bool persistent) {
    LightingObject light;
    light.buffers.emplace_back();
    light.buffers.back().init(markerMesh);
    light.position = position;
    light.color = color;
    light.persistent = persistent;
    if (castsShadows && shadowFramebuffer != 0) {
        light.shadowCubemap = createShadowCubemap();
    }
    lights.push_back(light);
}

void Engine::addLight(const char* modelPath, const glm::vec3& position,
                      const glm::vec3& color, const bool castsShadows,
                      const bool persistent) {
    LightingObject light;
    for (const auto& mesh : loadModelMeshes(modelPath)) {
        if (mesh.vertices.empty()) {
            continue;
        }
        light.buffers.emplace_back();
        light.buffers.back().init(mesh);
    }
    if (light.buffers.empty()) {
        std::cerr << "Light model contains no renderable meshes: " << modelPath << std::endl;
        return;
    }
    light.position = position;
    light.color = color;
    light.persistent = persistent;
    if (castsShadows && shadowFramebuffer != 0) {
        light.shadowCubemap = createShadowCubemap();
    }
    lights.push_back(light);
}

void Engine::clearDynamicObjects() {
    std::unordered_set<btRigidBody*> bodies;
    for (auto& object : sceneObjects) {
        if (object.body != nullptr && bodies.insert(object.body).second) {
            collisions.removeRigidBody(object.body);
        }
        object.body = nullptr;
    }
    sceneObjects.clear();
}

void Engine::clearLights() {
    for (auto it = lights.begin(); it != lights.end();) {
        if (it->persistent) {
            ++it;
            continue;
        }
        auto& light = *it;
        if (light.shadowCubemap != 0) {
            glDeleteTextures(1, &light.shadowCubemap);
            light.shadowCubemap = 0;
        }
        it = lights.erase(it);
    }
}

glm::vec3 Engine::getCameraPosition() const {
    return camera.getPosition();
}

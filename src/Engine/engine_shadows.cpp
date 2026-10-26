#include "Engine.h"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

}

bool Engine::initializeShadowMap() {
    shadowShader.createShaders(SHADER_RESOURCES_PATH "shadow_vertex.glsl",
                               SHADER_RESOURCES_PATH "shadow_geometry.glsl",
                               SHADER_RESOURCES_PATH "shadow_fragment.glsl");
    if (shadowShader.ID == 0) {
        return false;
    }

    glGenFramebuffers(1, &shadowFramebuffer);
    return shadowFramebuffer != 0;
}

GLuint Engine::createShadowCubemap() {
    GLuint shadowCubemap = 0;
    glGenTextures(1, &shadowCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap);
    for (unsigned int face = 0; face < 6; ++face) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT,
                     SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) {
        std::cerr << "Failed to create point-light shadow cubemap" << std::endl;
        glDeleteTextures(1, &shadowCubemap);
        return 0;
    }
    return shadowCubemap;
}

void Engine::renderShadowMap(const GLuint shadowCubemap, const glm::vec3& lightPosition,
                             const int width, const int height) {
    constexpr glm::vec3 directions[] = {
        { 1.0f,  0.0f,  0.0f}, {-1.0f,  0.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f}, { 0.0f, -1.0f,  0.0f},
        { 0.0f,  0.0f,  1.0f}, { 0.0f,  0.0f, -1.0f}
    };
    constexpr glm::vec3 ups[] = {
        {0.0f, -1.0f,  0.0f}, {0.0f, -1.0f,  0.0f},
        {0.0f,  0.0f,  1.0f}, {0.0f,  0.0f, -1.0f},
        {0.0f, -1.0f,  0.0f}, {0.0f, -1.0f,  0.0f}
    };
    const glm::mat4 projection =
        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, SHADOW_FAR_PLANE);
    glm::mat4 shadowMatrices[6];
    for (int face = 0; face < 6; ++face) {
        shadowMatrices[face] =
            projection * glm::lookAt(lightPosition, lightPosition + directions[face], ups[face]);
    }

    shadowShader.use();
    glUniformMatrix4fv(glGetUniformLocation(shadowShader.ID, "shadowMatrices[0]"), 6, GL_FALSE,
                       glm::value_ptr(shadowMatrices[0]));
    shadowShader.loadVector3("lightPos", lightPosition);
    glUniform1f(glGetUniformLocation(shadowShader.ID, "farPlane"), SHADOW_FAR_PLANE);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (const auto& object : terrainObjects) {
        shadowShader.loadMatrix("model", object.modelMatrix());
        object.uploadSkinning(shadowShader);
        object.buffers.draw();
    }
    for (const auto& object : sceneObjects) {
        shadowShader.loadMatrix("model", object.modelMatrix());
        object.uploadSkinning(shadowShader);
        object.buffers.draw();
    }
    if (car.isActive()) {
        for (const auto& carObject : car.getRenderObjects()) {
            shadowShader.loadMatrix("model", carObject.modelMatrix());
            carObject.uploadSkinning(shadowShader);
            carObject.buffers.draw();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

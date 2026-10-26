//
// Created by 777ni on 2026/09/16.
//

#include "SceneManager.h"

#include <glm/gtc/quaternion.hpp>

glm::mat4 SceneObject::modelMatrix() const {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model *= glm::mat4_cast(rotation);
    model = glm::scale(model, scale);
    model = glm::translate(model, localCenter);
    model *= glm::mat4_cast(localAnimationRotation);
    return glm::translate(model, -localCenter);
}

void SceneObject::uploadSkinning(const Shader& shader) const {
    const bool enabled = skinningEnabled && !jointMatrices.empty() &&
                         jointMatrices.size() <= 64;
    glUniform1i(glGetUniformLocation(shader.ID, "skinningEnabled"), enabled ? 1 : 0);
    if (enabled) {
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "jointMatrices[0]"),
                           static_cast<GLsizei>(jointMatrices.size()), GL_FALSE,
                           &jointMatrices.front()[0][0]);
    }
}

void SceneObject::draw(Shader& shader) {
    shader.loadMatrix("model", modelMatrix());
    uploadSkinning(shader);
    shader.loadVector4("baseColor", baseColor);
    glUniform1i(glGetUniformLocation(shader.ID, "alphaMasked"), alphaMasked ? 1 : 0);
    glUniform1f(glGetUniformLocation(shader.ID, "alphaCutoff"), alphaCutoff);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    buffers.draw();
}

void LightingObject::draw(Shader& shader) {
    glUniform1i(glGetUniformLocation(shader.ID, "skinningEnabled"), 0);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    shader.loadMatrix("model", model);
    for (const auto& buffer : buffers) {
        buffer.draw();
    }
}

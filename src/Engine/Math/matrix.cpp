//
// Created by 777ni on 2026/09/11.
//

#include "matrix.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

void Matrix::Translate(glm::vec3 newPos) {
    model = glm::translate(model, newPos);
}

void Matrix::Scale(glm::vec3 newScale) {
    model = glm::scale(model, newScale);
}

void Matrix::Rotate(float degrees, glm::vec3 Axis) {
    model = glm::rotate(model, glm::radians(degrees), Axis);
}

Matrix::Matrix(int WIDTH, int HEIGHT) {
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 1000.0f);
}

//
// Created by 777ni on 2026/09/11.
//

#ifndef RACECAR_MATRIX_H
#define RACECAR_MATRIX_H
#include <glad/glad.h>
#include <glm/glm.hpp>


class Matrix {
    public:
    Matrix(int WIDTH, int HEIGHT);
    glm::mat4 model{1.0f};
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};

    void Translate(glm::vec3 newPos);
    void Rotate(float degrees, glm::vec3 axis);
    void Scale(glm::vec3 newScale);
};


#endif // RACECAR_MATRIX_H

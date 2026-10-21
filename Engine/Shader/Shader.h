//
// Created by 777ni on 2026/09/10.
//

#ifndef RACECAR_SHADER_H
#define RACECAR_SHADER_H
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <glm/fwd.hpp>

class Shader {
public:
    GLuint ID = 0;

    void createShaders(const char* vertex_path, const char* fragment_path);
    void createShaders(const char* vertex_path, const char* geometry_path, const char* fragment_path);
    void use();
    void loadMatrix(const char * name, const glm::mat4 & matrix);
    void loadVector3(const char * name, const glm::vec3 & vector);
    void loadVector4(const char * name, const glm::vec4 & vector);
    void loadTexture(const char* name, int textureUnit);

};
#endif // RACECAR_SHADER_H

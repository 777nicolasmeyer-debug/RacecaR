//
// Created by 777ni on 2026/09/10.
//
#include "Shader.h"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>

void Shader::createShaders(const char* vertex_path, const char* fragment_path) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::badbit|std::ifstream::failbit);
    fShaderFile.exceptions(std::ifstream::badbit|std::ifstream::failbit);

    try {

        vShaderFile.open(vertex_path);
        fShaderFile.open(fragment_path);

        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "Failed to read Shader Files" << std::endl;
    }
    const char* vSource = vertexCode.c_str();
    const char* fSource = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vSource, nullptr);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::cerr << "Vertex shader compilation failed" << std::endl;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fSource, nullptr);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::cerr << "Fragment shader compilation failed" << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        std::cerr << "Program linking failed" << std::endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::createShaders(const char* vertex_path, const char* geometry_path, const char* fragment_path) {
    auto compileShader = [](GLenum type, const char* path) {
        std::string source;
        std::ifstream file(path);
        if (!file) {
            std::cerr << "Failed to read shader file: " << path << std::endl;
            return GLuint{0};
        }
        std::stringstream stream;
        stream << file.rdbuf();
        source = stream.str();

        const char* sourceText = source.c_str();
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &sourceText, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "Shader compilation failed (" << path << "): " << infoLog << std::endl;
            glDeleteShader(shader);
            return GLuint{0};
        }
        return shader;
    };

    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertex_path);
    const GLuint geometry = compileShader(GL_GEOMETRY_SHADER, geometry_path);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragment_path);
    if (vertex == 0 || geometry == 0 || fragment == 0) {
        if (vertex != 0) glDeleteShader(vertex);
        if (geometry != 0) glDeleteShader(geometry);
        if (fragment != 0) glDeleteShader(fragment);
        ID = 0;
        return;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, geometry);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    GLint success = GL_FALSE;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(ID, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shadow shader program linking failed: " << infoLog << std::endl;
        glDeleteProgram(ID);
        ID = 0;
    }

    glDeleteShader(vertex);
    glDeleteShader(geometry);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::loadVector4(const char * name, const glm::vec4 & vector) {
    glUniform4fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vector));
}
void Shader::loadMatrix(const char *name, const glm::mat4 &matrix) {
    glGetUniformLocation(ID, name);
    glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::loadVector3(const char * name, const glm::vec3 & vector) {
    glGetUniformLocation(ID, name);
    glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vector));
}

void Shader::loadTexture(const char* name, int textureunit) {
    unsigned int location = glGetUniformLocation(ID, name);
    glUniform1i(location, textureunit);
}

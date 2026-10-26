//
// Created by 777ni on 2026/09/10.
//
#include "VAO_VBO.h"

void VAO_VBO::init(const MeshData& meshData) {
    indexType = meshData.indexType;
    size_t indexSize = 0;
    switch (meshData.indexType) {
        case GL_UNSIGNED_SHORT: indexSize = sizeof(unsigned short); break;
        case GL_UNSIGNED_INT:   indexSize = sizeof(unsigned int);   break;
        case GL_UNSIGNED_BYTE:  indexSize = sizeof(unsigned char);  break;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, meshData.vertices.size() * sizeof(float), meshData.vertices.data(), GL_STATIC_DRAW);

    if (meshData.skinned && !meshData.skinAttributes.empty()) {
        glGenBuffers(1, &skinVbo);
        glBindBuffer(GL_ARRAY_BUFFER, skinVbo);
        glBufferData(GL_ARRAY_BUFFER, meshData.skinAttributes.size() * sizeof(float),
                     meshData.skinAttributes.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              reinterpret_cast<void *>(4 * sizeof(float)));
        glEnableVertexAttribArray(4);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * indexSize, meshData.indices.data(), GL_STATIC_DRAW);

    // Position: starts at 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal: starts right after position (3 floats = 12 bytes)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV: starts after position+normal (6 floats = 24 bytes)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    indexCount = static_cast<GLsizei>(meshData.indices.size());
}

void VAO_VBO::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, indexType, 0);



}


#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

in vec3 WorldPos[];
out vec3 FragPos;

uniform mat4 shadowMatrices[6];

void main() {
    for (int face = 0; face < 6; ++face) {
        gl_Layer = face;
        for (int vertex = 0; vertex < 3; ++vertex) {
            FragPos = WorldPos[vertex];
            gl_Position = shadowMatrices[face] * vec4(FragPos, 1.0);
            EmitVertex();
        }
        EndPrimitive();
    }
}

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aJoints;
layout (location = 4) in vec4 aWeights;

out vec2 texCoord;
out vec3 Normal;
out vec3 Pos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool skinningEnabled;
uniform mat4 jointMatrices[64];
void main() {
    vec4 position = vec4(aPos, 1.0);
    vec3 normal = aNormal;
    if (skinningEnabled) {
        ivec4 joints = ivec4(aJoints);
        float totalWeight = dot(aWeights, vec4(1.0));
        if (totalWeight > 0.0) {
            vec4 weights = aWeights / totalWeight;
            mat4 skin = jointMatrices[joints.x] * weights.x +
                        jointMatrices[joints.y] * weights.y +
                        jointMatrices[joints.z] * weights.z +
                        jointMatrices[joints.w] * weights.w;
            position = skin * position;
            normal = mat3(skin) * normal;
        }
    }
    gl_Position = projection * view * model * position;
    texCoord = aTexCoord;
    Normal = mat3(transpose(inverse(model))) * normal;
    Pos = vec3(model * position);
}

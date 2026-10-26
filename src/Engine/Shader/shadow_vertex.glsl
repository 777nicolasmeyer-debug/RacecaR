#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec4 aJoints;
layout (location = 4) in vec4 aWeights;

uniform mat4 model;
uniform bool skinningEnabled;
uniform mat4 jointMatrices[64];

out vec3 WorldPos;

void main() {
    vec4 position = vec4(aPos, 1.0);
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
        }
    }
    WorldPos = vec3(model * position);
    gl_Position = vec4(WorldPos, 1.0);
}

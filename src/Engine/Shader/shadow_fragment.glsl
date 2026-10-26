#version 330 core

in vec3 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main() {
    gl_FragDepth = length(FragPos - lightPos) / farPlane;
}

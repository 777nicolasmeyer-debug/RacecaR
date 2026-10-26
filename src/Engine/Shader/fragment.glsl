#version 330 core

in vec2 texCoord;
in vec3 Normal;
in vec3 Pos;
out vec4 frag;

uniform sampler2D tex;
uniform samplerCube shadowCube;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec4 baseColor;
uniform float farPlane;
uniform float alphaCutoff;
uniform int shadowsEnabled;
uniform int ambientEnabled;
uniform int alphaMasked;

const vec3 shadowSampleDirections[20] = vec3[](
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0,  1, -1), vec3( 0, -1, -1)
);

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - Pos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 viewDir = normalize(viewPos - Pos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    float shadow = 0.0;
    if (shadowsEnabled != 0) {
        vec3 fragToLight = Pos - lightPos;
        float currentDepth = length(fragToLight);
        float bias = max(0.8 * (1.0 - dot(norm, lightDir)), 0.35);
        float diskRadius = 0.01 * (1.0 + currentDepth / farPlane);
        for (int i = 0; i < shadowSampleDirections.length(); ++i) {
            float closestDepth =
                texture(shadowCube, fragToLight + shadowSampleDirections[i] * diskRadius).r * farPlane;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
        shadow /= float(shadowSampleDirections.length());
    }

    vec3 ambient = ambientEnabled != 0 ? 0.1 * lightColor : vec3(0.0);
    vec3 diffuse = diff * lightColor;
    vec3 specular = spec * lightColor;
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    vec4 color = texture(tex, texCoord) * baseColor * vec4(lighting, 1.0);
    if (alphaMasked != 0 && color.a < alphaCutoff) {
        discard;
    }
    frag = color;
}

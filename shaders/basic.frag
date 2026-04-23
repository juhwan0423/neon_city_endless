#version 330 core

in vec3 WorldPos;

out vec4 FragColor;

uniform vec3 baseColor;
uniform vec3 emissiveColor;
uniform float emissiveStrength;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;

void main() {
    float distanceToCamera = distance(cameraPos, WorldPos);
    float fogFactor = 1.0 - exp(-distanceToCamera * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    float heightTint = clamp(WorldPos.y * 0.04 + 0.55, 0.25, 1.0);
    vec3 shaded = baseColor * heightTint;
    vec3 color = shaded + emissiveColor * emissiveStrength;
    color = mix(color, fogColor, fogFactor);

    FragColor = vec4(color, 1.0);
}

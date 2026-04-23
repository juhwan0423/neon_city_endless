#version 330 core

in vec3 WorldPos;
in vec3 LocalPos;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec3 baseColor;
uniform vec3 emissiveColor;
uniform float emissiveStrength;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float bloomThreshold;
uniform float timeSeconds;
uniform int materialType;

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    float distanceToCamera = distance(cameraPos, WorldPos);
    float fogFactor = 1.0 - exp(-distanceToCamera * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    float heightTint = clamp(WorldPos.y * 0.04 + 0.55, 0.25, 1.0);
    vec3 shaded = baseColor * heightTint;
    vec3 hdrColor = shaded + emissiveColor * emissiveStrength;

    if (materialType == 1) {
        float topMask = smoothstep(0.10, 0.45, LocalPos.y);
        float centerGlow = exp(-abs(WorldPos.x) * 1.8);
        float edgeGlow = exp(-abs(abs(WorldPos.x) - 3.45) * 5.5);
        float streaks = 0.5 + 0.5 * sin(WorldPos.z * 0.55 - timeSeconds * 3.8);
        float puddleNoise = hash12(floor(WorldPos.xz * vec2(0.75, 0.16)));
        float puddleMask = smoothstep(0.62, 0.92, puddleNoise) * topMask;

        vec3 coolReflection = vec3(0.08, 0.95, 1.0) * (0.18 + 0.28 * centerGlow);
        vec3 warmReflection = vec3(1.0, 0.22, 0.58) * (0.06 + 0.22 * edgeGlow * streaks);
        vec3 amberReflection = vec3(1.0, 0.62, 0.18) * (0.04 + 0.16 * centerGlow * (1.0 - streaks));
        vec3 reflection = coolReflection + warmReflection + amberReflection;

        shaded += reflection * (0.18 * topMask + 0.55 * puddleMask);
        hdrColor += reflection * (0.08 * topMask + 0.20 * puddleMask);
    }

    vec3 color = mix(hdrColor, fogColor, fogFactor);
    float brightness = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));

    FragColor = vec4(color, 1.0);
    BrightColor = brightness > bloomThreshold ? vec4(hdrColor, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}

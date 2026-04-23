#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D bloomBlurTexture;
uniform float exposure;
uniform float bloomStrength;
uniform float timeSeconds;

float grain(vec2 uv) {
    return fract(sin(dot(uv + timeSeconds * 0.01, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    vec3 hdrColor = texture(sceneTexture, TexCoords).rgb;
    vec3 bloom = texture(bloomBlurTexture, TexCoords).rgb * bloomStrength;
    vec3 combined = hdrColor + bloom;

    float horizon = smoothstep(0.15, 0.95, TexCoords.y);
    combined += vec3(0.010, 0.006, 0.022) * (1.0 - horizon) * 0.8;

    vec3 mapped = vec3(1.0) - exp(-combined * exposure);
    mapped = pow(mapped, vec3(1.0 / 2.2));

    mapped *= vec3(0.96, 0.98, 1.03);
    mapped += bloom * vec3(0.06, 0.025, 0.09);

    vec2 centered = TexCoords * 2.0 - 1.0;
    float vignette = 1.0 - dot(centered, centered) * 0.22;
    vignette = clamp(vignette, 0.76, 1.0);
    mapped *= vignette;

    mapped += (grain(TexCoords) - 0.5) * 0.02;
    mapped = clamp(mapped, 0.0, 1.0);

    FragColor = vec4(mapped, 1.0);
}

# Neon City Starter

Minimal OpenGL starter for a procedural neon city project.

## Features in this starter

- GLFW window and input handling
- GLEW-based OpenGL loader
- GLM camera and matrix math
- Shader loading from external files
- Procedural road segments rendered ahead of the camera
- Rule-based building variation with emissive neon signs and windows

## Build

```bash
cmake -S . -B build
cmake --build build
./build/neon_city
```

## Controls

- `W/A/S/D`: move
- `Q/E`: down/up
- Mouse: look around
- Mouse wheel: zoom
- `Esc`: quit

## Suggested next steps

1. Replace the simple emissive pass with HDR + bloom.
2. Turn road cubes into a curved segment mesh generator.
3. Swap static box facades for instanced facade modules.
4. Add wet-road reflections or screen-space post effects.

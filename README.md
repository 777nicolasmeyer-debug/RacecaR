# RacecaR

**A small 3D driving sandbox built with C++, OpenGL, and Bullet Physics.**

RacecaR renders a glTF-based F1 car and track in a real-time scene. Drive with a
manual gearbox, explore on foot, and add lights to the scene. The project also
includes a host-driven `Engine` API for applications that want to own their
window and OpenGL lifecycle.

> **Current state:** CMake builds the GLFW sample application (`RacecaR`). The
> engine source is organized around a host-driven API, but is not yet exported
> as a standalone CMake library.

## Features

- OpenGL 3.3 rendering with point lights and shadow mapping
- glTF scene loading, including node transforms and model caching
- Bullet-powered collision and vehicle physics
- Switchable first-person camera and F1 car
- Named car parts animate steering and wheel rotation when supported by the model
- Eight-speed manual gearbox with RPM and gear shown in the window title
- Configurable vehicle handling in `Engine/Car/CarConfig.h`

### RB18 wheel setup

The car uses Bullet's four-wheel raycast vehicle for steering, suspension, grip,
and braking. It fits the physics wheel centers and radius from the four RB18
tire meshes, using the standard `+X` right, `+Y` up, `+Z` forward vehicle frame.
In Blender, keep each tire as a separate front/rear and left/right node, and preserve names
containing `Front` or `Back` and `Tire`; matching rim, disk, and hub nodes follow
their corresponding wheel. If you change the model's orientation or export
scale, update handling values in `Engine/Car/CarConfig.h`. Tune steering angles,
suspension stiffness/damping, and suspension travel there; changing the Blender
body shape alone will not change how the physics car turns.

## Controls

| Action | On foot | In the car |
| --- | --- | --- |
| Move / accelerate | `W` | `W` |
| Move backward / brake | `S` | `S` or `Space` |
| Strafe / steer | `A` / `D` | `A` / `D` |
| Move vertically | `Space` / `Left Shift` | N/A |
| Shift gear | N/A | `E` up / `Q` down |
| Look around | Mouse | Camera follows the car |
| Toggle car | `P` | `P` |
| Spawn a light | `L` | `L` |
| Clear dynamic objects and lights | `R` | `R` |
| Quit | `Esc` | `Esc` |

## Build and run

### Requirements

- CMake 3.20 or newer
- A C++23-capable compiler
- A graphics driver that supports OpenGL 3.3

Dependencies are included under `Engine/external`. The build uses bundled GLFW
and Bullet sources, GLAD, GLM, TinyGLTF, and the SFML 3 audio package.

### Build

From the repository root:

```sh
cmake -S . -B build
cmake --build build --config Release
```

### Run

The build defines `RESOURCES_PATH` as the source directory, so the sample can
find its models and shaders regardless of the process working directory. Add a
resource path by appending its repository-relative location, for example
`RESOURCES_PATH "assets/Light.glb"`.

For a single-configuration generator:

```sh
./build/RacecaR
```

For a multi-configuration generator such as Visual Studio:

```powershell
.\build\Release\RacecaR.exe
```

On Windows, ensure the SFML runtime DLLs in
`Engine/external/SFML/SFML-3.1.0/bin` can be found by the application (for
example, by adding that directory to `PATH`).

## Using the engine API

`Engine/Engine.h` contains the public interface. The host application must
create and activate an OpenGL 3.3 context, initialize GLAD, then initialize the
engine while that context is current. Each frame, pass input and elapsed time
to the engine, render using the framebuffer dimensions, and let the host
present its window:

```cpp
Engine engine;
if (!engine.initialize()) {
    // Handle initialization failure.
}

engine.addStaticModel("track.glb", glm::vec3(0.0f));

// In the host application's frame loop:
Engine::CameraInput input; // Map the host's key state into these fields.
engine.handleKeyboardInput(input, deltaTime);
engine.update(deltaTime);
engine.render(framebufferWidth, framebufferHeight);
// Present/swap buffers in the host application.
```

See [`src/main.cpp`](src/main.cpp) for the complete GLFW host, input mapping,
and example scene setup. The engine API also supports adding static and dynamic
models, lights, and procedural mesh data.

## Project layout

| Path | Contents |
| --- | --- |
| `src/main.cpp` | GLFW sample application and frame loop |
| `Engine/` | Rendering, camera, scene, loading, and physics code |
| `Engine/Car/` | Vehicle behavior and handling configuration |
| `Engine/Shader/` | GLSL shaders |
| `assets/` | Track, car, light, collision, and audio assets |
| `Engine/external/` | Bundled third-party dependencies |

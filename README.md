# RacecaR

`Engine` is a renderer/physics component; it does not create a window, poll events,
or swap buffers. The host application owns the OpenGL context and drives the engine,
so it can use GLFW, Qt, or an ImGui host:

1. Create and activate an OpenGL 3.3 context, then initialize GLAD.
2. Call `Engine::initialize()` once while that context is current.
3. Add model files with `addStaticModel`, `addDynamicModel`, and `addLight`.
   The engine loads all meshes in the selected glTF scene, applies node transforms,
   and caches loaded model data. Mesh-level methods are also available for custom
   procedural assets.
4. Each frame, translate the host's keyboard state to `Engine::CameraInput`, call
   `handleKeyboardInput`, `update(deltaTime)`, and `render(width, height)`.
5. Draw the GUI after `render`, then let the host swap/present its buffers.

`src/main.cpp` is the GLFW example host and contains the sample game's asset setup
and spawn-key bindings. A Qt host can use the same engine calls from its
`QOpenGLWidget` lifecycle without adopting the sample application's GLFW loop.

The sample F1 car uses W to accelerate, S or Space to brake, A/D to steer,
E to shift up, and Q to shift down. Its manual eight-speed gearbox shifts once per key press,
and the window title displays the current gear and RPM. The car coasts with
gradual drag instead of rapidly losing speed when throttle is released.
`Engine::getPlayerRPM()` and `Engine::getPlayerGear()` are available to a host
that wants to draw its own dashboard. Gear speed limits, acceleration rates,
gear RPM multipliers, coast drag, tire grip, yaw rate, downforce, and brake force
are tuned in `Engine/Car/CarConfig.h`. The RPM display follows vehicle speed but does not
control vehicle speed or acceleration.

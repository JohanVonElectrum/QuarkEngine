# Architecture Overview

Quark is a minimal 3D game engine written in C23. The architecture is deliberately small and consists of a thin core layer plus a complete Vulkan renderer with no additional graphics API support.

## Layering (bottom-up)

- **Platform primitives** come entirely from the `cstdlib` library (`<cstdlib/mem.h>`, `<cstdlib/thread.h>`, `<cstdlib/clock.h>`, plus `cstdlib_init`). Quark contains almost no operating-system-specific code.

- **Core** (`quark/src/core`):
  - `engine.c` – entry-point initialization that calls `cstdlib_init` followed by the tracing subsystem.
  - `tracing.c` + `event.c` – asynchronous logging and tracing-span support. A background worker thread consumes a lock-free single-producer/single-consumer power-of-two ring buffer. The queue uses C11 atomics; the worker uses cstdlib threading and monotonic clock facilities.
  - `application.c` – singleton application object (`static Application s_application`) whose lifecycle is tracked with simple bit flags. Owns the camera and optional window.
  - `window.c` – minimal GLFW wrapper responsible for window creation, framebuffer-resize forwarding, event polling, and `VkSurfaceKHR` creation.
  - `camera.c` – view and right-handed perspective matrix calculation using cglm. The resulting matrix is supplied to the renderer as a single push constant.

- **Renderer** (`quark/src/renderer`):
  - Implemented 100% in Vulkan 1.4. The files are `backend.c` (instance, optional debug messenger, shader loading, the single graphics pipeline, frame recording, acquire/present logic), `device.c` (physical-device selection, queue families, logical device), and `swapchain.c` (swapchain, depth image, render pass, framebuffers, synchronization).
  - GLFW is used only to obtain required Vulkan instance extensions, create the native window, and produce the surface. Every other Vulkan operation is performed with direct Vulkan calls.
  - A single hardcoded pipeline exists (no vertex input, 36 vertices drawn with `vkCmdDraw`). The camera matrix is pushed per frame.

- **Public headers** (`quark/include/quark`): `entrypoint.h`, and the `core/` subdirectory (`application.h`, `camera.h`, `engine.h`, `window.h`, `assert.h`, `log.h`), plus `world/chunk.h`.

- **Build-time** custom targets compile the two shaders (`quark/assets/shaders/obb.vert` and `obb.frag`) into `.spv` files placed under the runtime `assets/shaders` directory.

The engine currently has no asset system, scene graph, material system, or frame graph. The main loop is a straightforward poll-and-render loop.

## Project Map

- `quark/` builds `quark` as a shared library; `testbed/` builds the host application that links it.
- The engine depends on `cstdlib` (sibling project providing memory, threading, clock, and other low-level primitives). During configuration the build looks for a local cstdlib checkout at the sibling path `../cstdlib` (relative to the repository root) or `../../cstdlib` (relative to `quark/`). If not found it falls back to fetching from GitHub via FetchContent.
- Public API surface is under `quark/include/quark/*`; engine internals live under `quark/src/*`.
- Entry point ownership is inverted: application code provides `init_application(...)`, the engine provides `main(...)` in `quark/include/quark/entrypoint.h`.
- Rendering is implemented exclusively against Vulkan 1.4; there is no multi-backend abstraction.

## Runtime Architecture and Data Flow

- The fixed startup sequence is defined in `quark/include/quark/entrypoint.h`:
  `init_quark` → `init_application` (user) → `create_application` → `run_application` → `destroy_application` → `shutdown_quark`.
- `init_quark` (`quark/src/core/engine.c`) first initializes cstdlib and then starts the tracing worker thread plus its lock-free event queue. Only after this point are the `QUARK_LOG_*` macros safe to use.
- `create_application` (`quark/src/core/application.c`) enforces a singleton and records lifecycle state in bit flags. When the window mode is not headless it brings up GLFW windowing, the Vulkan renderer, and creates the window (which also initializes the swapchain and pipeline).
- In `run_application` the loop repeatedly calls `windowing_poll_events`. When not headless it also calls `render_renderer_frame`, which performs the classic acquire-record-submit-present cycle and pushes the camera's view-projection matrix as a push constant.
- `render_renderer_frame` records a command buffer that clears the color and depth attachments, binds the pipeline, pushes the matrix, draws 36 vertices, and ends the render pass.
- Resize is handled by setting a flag that triggers swapchain recreation on the next present failure or explicit request.
- Shutdown tears down the pipeline, swapchain, device resources, window, renderer backend, and windowing system (in reverse order of creation) before stopping tracing and cstdlib.

## Rendering and Platform Boundaries

- The renderer is Vulkan-only. All rendering code lives under `quark/src/renderer` with no indirection or backend enumeration.
- GLFW responsibilities are limited to cross-platform window management and Vulkan surface creation. No OpenGL, no other APIs.
- `quark/CMakeLists.txt` performs the following:
  - Detects and adds a local cstdlib checkout (shared library) when present.
  - Fetches GLFW (shared, examples/tests/docs disabled) and cglm via FetchContent.
  - Requires the system Vulkan SDK (`find_package(Vulkan REQUIRED)`).
  - Invokes the SDK's `glslc` compiler via custom commands to produce the two SPIR-V files and wires the output directory into the engine via a compile definition.
- The only platform-specific construct inside the engine is the `QUARK_PLATFORM_WINDOWS` define used solely to select `__debugbreak()` inside the assertion macros. All memory allocation, threading, and timing are obtained from cstdlib headers and functions.

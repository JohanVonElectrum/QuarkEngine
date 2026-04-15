# AGENTS Guide for QuarkEngine

## Project map
- `quark/` builds `quark` as a shared library; `testbed/` builds the host app that links it.
- Public API surface is under `quark/include/quark/*`; engine internals live under `quark/src/*`.
- Entry point ownership is inverted: app code provides `init_application(...)`, engine provides `main(...)` in `quark/include/quark/entrypoint.h`.

## Build and run workflow
- Configure (Debug, C23):
```powershell
cmake -S D:\projects\QuarkEngine -B D:\projects\QuarkEngine\cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```
- Build:
```powershell
cmake --build D:\projects\QuarkEngine\cmake-build-debug
```
- Run testbed:
```powershell
D:\projects\QuarkEngine\cmake-build-debug\bin\testbed.exe
```
- Tests: `ctest --test-dir D:\projects\QuarkEngine\cmake-build-debug --output-on-failure` (currently no tests are registered).

## Runtime architecture and data flow
- Startup order is fixed in `quark/include/quark/entrypoint.h`: `init_quark` -> `init_application` -> `create_application` -> `run_application` -> `destroy_application` -> `shutdown_quark`.
- `init_quark` (`quark/src/core/engine.c`) initializes main thread state and tracing before normal logging.
- `create_application` (`quark/src/core/application.c`) is singleton-based (`static Application s_application`) and uses bit flags for lifecycle state.
- Window mode drives renderer selection: `GRAPHICS_MODE_NONE` is headless; graphics mode maps to backend via `create_info->window.mode - 1`.
- Main loop is event-poll only right now (`windowing_poll_events`, `window_should_close`) with no frame graph yet.

## Rendering and platform boundaries
- Backend dispatch is centralized in `quark/src/renderer/backend.c` with a `RendererBackendKind` switch.
- Vulkan backend lives in `quark/src/renderer/vulkan/*`; GLFW provides Vulkan instance extensions and creates surfaces.
- CMake fetches GLFW (`FetchContent`) and requires system Vulkan SDK (`find_package(Vulkan REQUIRED)`) in `quark/CMakeLists.txt`.
- Platform implementations are file-suffixed (`*_win32.c`) and compiled conditionally from `quark/CMakeLists.txt`.

## Code conventions specific to this repo
- The codebase is C with C23 features enabled (`CMAKE_C_STANDARD 23`), including `nullptr`, `thread_local`, `constexpr`, and designated initializers.
- Primitive aliases/macros are standardized in `quark/include/quark/primitives.h` (`QUARK_B8`, `QUARK_U32`, `BIT(...)`).
- Error handling pattern: `QUARK_ASSERT_RETURN(...)` / `QUARK_ASSERT_X(...)` from `quark/include/quark/core/assert.h` for guard clauses plus optional cleanup blocks.
- Memory must flow through `quark_mem_*` wrappers (`quark/src/platform/memory.h`) instead of direct `malloc/free`.
- Logging is asynchronous through tracing worker queue (`quark/src/core/tracing.c` + `quark/src/core/event.c`); do not assume synchronous log flush timing.

## Safe extension patterns
- To add a renderer backend, extend `RendererBackendKind` and all switch sites in `quark/src/renderer/backend.c`, then mirror the Vulkan backend header pattern.
- To add app behavior, implement/extend `init_application` in the app (`testbed/src/main.c`) rather than editing engine entrypoint flow.
- To add a new platform, provide `platform/*_<platform>.c` implementations and wire them in `quark/CMakeLists.txt` under platform defines.


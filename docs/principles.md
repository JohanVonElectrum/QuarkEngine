# Design Principles and Extension Patterns

## Core Principles

- **Minimalism**: The engine deliberately keeps a very small core. Avoid unnecessary abstraction layers unless they provide clear value.
- **Platform via cstdlib**: All low-level platform concerns (memory, threading, time, etc.) are delegated to the sibling `cstdlib` project. Quark should not grow its own platform layer.
- **Inverted entry point**: Application code owns `init_application(...)`. The engine owns `main(...)` (see `quark/include/quark/entrypoint.h`).
- **Vulkan only**: Rendering is implemented exclusively against Vulkan 1.4. There is no multi-backend abstraction.

## Safe Extension Patterns

- **Adding another graphics API**: Requires introducing a renderer abstraction layer, duplicating device/swapchain/pipeline logic, and updating call sites in the window and application modules. Because the current renderer is intentionally monolithic and small, such an extension is a significant undertaking.
- **Application-specific behavior**: Belongs in the caller's `init_application` implementation (and the other optional lifecycle functions). The engine entry-point flow in `entrypoint.h` is not intended to be modified.
- **Platform or OS-specific code**: Must not be added inside the `quark/` tree. Any required low-level primitives should be contributed to the `cstdlib` project first and then consumed from Quark.
- **New core engine systems**: Should be placed under `src/core` (or a clearly separated new directory) and must route all allocation, threading, and timing through the cstdlib API. Keep additions to the public header surface minimal.

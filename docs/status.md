# Current Project Status

## Major Completed Work

- Full migration of primitive types from custom `QUARK_*` aliases to cstdlib equivalents (`u32_t`, `b8_t`, `usize_t`, `f32_t`, etc.).
- Removal of `quark/include/quark/primitives.h` and `quark/include/quark/api.h`.
- Autogeneration of `quark/api.h` using CMake `generate_export_header` (following the same pattern as cstdlib).
- Strict include ordering rules enforced across the codebase.
- Comprehensive nullability annotations using `<cstdlib/nullability.h>` + `NONNULL_ARGS()` rules.
- Documentation pass on public and internal headers in cstdlib style.
- Significant cleanup and accuracy improvements to existing documentation comments.

## Current State (as of latest changes)

- The engine builds cleanly.
- Testbed runs.
- Code conventions (especially includes and nullability) are being actively enforced.
- The voxel / dynamic block world rendering work is still in the planning / early implementation phase.

## Known Limitations

- Renderer is still relatively minimal (hardcoded pipeline + basic swapchain management).
- No asset system, scene graph, or advanced rendering features yet.
- Documentation is still being refined for accuracy.

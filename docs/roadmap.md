# Roadmap

## Near Term

- Complete the documentation accuracy review across all headers.
- Continue enforcing and refining code conventions (include ordering, nullability, etc.).
- Further improvements to the voxel / chunk rendering infrastructure (see `include/quark/world/chunk.h`).

## Medium Term

- Proper GPU resource management for dynamic chunk meshes (staging rings, persistent buffers, compute-assisted culling/indirect drawing evaluation).
- Greedy meshing implementation for blocks.
- Camera frustum culling integrated with the chunk system.
- More advanced material / lighting support for voxel worlds.

## Long Term / Aspirational

- Larger scale dynamic block world support with efficient streaming and editing.
- More complete engine features (asset pipeline, job system, better profiling, etc.) while keeping the core minimal.
- Potential exploration of mesh shaders or other modern Vulkan features for voxel rendering.

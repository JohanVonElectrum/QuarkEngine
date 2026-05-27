# Open Tasks and TODOs

## High Priority

- Review and correct all documentation comments for accuracy against actual implementation (ongoing).
- Continue the nullability annotation effort across remaining headers.
- Ensure all new code follows the latest include ordering and `NONNULL_ARGS` rules.

## Medium Priority

- Evaluate and implement efficient GPU caching strategies for chunk meshes.
- Implement or integrate greedy meshing for block models.
- Add frustum culling helpers that work well with the chunk system.
- Improve test coverage (currently very limited).

## Low Priority / Future

- Background meshing using cstdlib threading + arenas.
- More advanced occlusion culling or hierarchical techniques.
- Texture atlas / material system suitable for large voxel worlds.
- Profiling and optimization pass on the current renderer path.

## Documentation Tasks

- Keep `status.md` and `roadmap.md` up to date as major work items progress.
- Consider adding more examples in the code conventions document.

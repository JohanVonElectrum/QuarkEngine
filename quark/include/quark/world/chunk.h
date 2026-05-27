#pragma once

#include <cstdlib/common.h>

/**
 * Chunk coordinate in the world grid.
 * Each chunk occupies a 16x16x16 block volume (our core rendering primitive).
 */
typedef struct
{
    i32_t x;
    i32_t y;
    i32_t z;
} ChunkCoord;

/**
 * Packed vertex format for a single vertex of a cuboid (after transform).
 *
 * Positions (x, y, z) are in **chunk-local space** — i.e. relative to the
 * origin returned by chunk_origin_world(). Because a cuboid inside a block
 * inside a chunk is not required to be grid-aligned and can be arbitrarily
 * rotated and scaled (via its mat4 transform), each coordinate can be any
 * float value. No axis-alignment or integer snapping can be assumed.
 *
 * Normals are deliberately not stored. For each triangle the GPU has the
 * three vertex positions and can compute the normal via cross product
 * (or dFdx/dFdy in the fragment shader). This is the correct and memory-
 * efficient approach when faces are not axis-aligned.
 *
 * The attrs word uses the following packing (kept for now):
 *   bits  0-15 : uv (commonly two 8-bit coordinates for atlas)
 *   bits 16-23 : light value (0-255, e.g. sky + block light combined)
 *   bits 24-31 : color tint / material id (0-255)
 */
typedef struct
{
    f32_t x, y, z;
    u32_t attrs;
} VoxelVertex;

/**
 * CPU-side description of a fully meshed chunk ready for GPU upload.
 *
 * The vertices here are the final baked result after applying any Cuboid
 * mat4 transforms. Positions are chunk-local floats; no grid alignment or
 * axis alignment is assumed. Normals are computed on the GPU.
 *
 * The pointed-to arrays are owned by the caller (typically backed by a
 * cstdlib arena that will be reset after the upload is submitted).
 * The voxel renderer will copy the data into staging buffers.
 */
typedef struct
{
    const VoxelVertex* verts;
    u32_t vert_count;

    const u32_t* indices;
    u32_t index_count;

    ChunkCoord coord;
} ChunkMesh;

/**
 * Opaque handle to a cached GPU-resident chunk mesh owned by the voxel
 * renderer. Treat as an opaque token only (never inspect the bits).
 */
typedef struct
{
    u64_t id;
} ChunkHandle;

static constexpr ChunkHandle CHUNK_HANDLE_INVALID = {0};

static INLINE ChunkCoord chunk_coord(const i32_t x, const i32_t y, const i32_t z) {
    return (ChunkCoord){x, y, z};
}

static INLINE b8_t chunk_coords_equal(const ChunkCoord a, const ChunkCoord b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

static INLINE void chunk_origin_world(const ChunkCoord c, i32_t out[3]) {
    out[0] = c.x * 16;
    out[1] = c.y * 16;
    out[2] = c.z * 16;
}

static INLINE u16_t voxel_vertex_uv(const VoxelVertex* v) {
    return (u16_t) (v->attrs & 0xFFFF);
}

static INLINE u8_t voxel_vertex_light(const VoxelVertex* v) {
    return (u8_t) ((v->attrs >> 16) & 0xFF);
}

static INLINE u8_t voxel_vertex_color(const VoxelVertex* v) {
    return (u8_t) ((v->attrs >> 24) & 0xFF);
}

/** Packs uv + light + color/material into the attribute word. */
static INLINE u32_t voxel_pack_attrs(u16_t uv, u8_t light, u8_t color) {
    return ((u32_t) uv & 0xFFFF) |
           ((u32_t) light << 16) |
           ((u32_t) color << 24);
}

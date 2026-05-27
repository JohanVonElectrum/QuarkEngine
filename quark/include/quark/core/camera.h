#pragma once

#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>
#include <cglm/types.h>

/**
 * Simple perspective camera definition.
 *
 * Used by the renderer and application to compute view-projection matrices.
 * The camera uses a right-handed coordinate system.
 */
typedef struct
{
    vec3 position;
    vec3 forward;
    vec3 up;
    f32_t fov_y_radians;
    f32_t near_plane;
    f32_t far_plane;
} Camera;

/**
 * A view frustum represented by 6 planes.
 *
 * Each plane is stored as vec4 (nx, ny, nz, d), where (nx, ny, nz) is the
 * inward-pointing normal and d is the signed distance from the origin
 * (plane equation: dot(normal, p) + d = 0).
 *
 * Plane ordering (by index):
 *   0 = left
 *   1 = right
 *   2 = bottom
 *   3 = top
 *   4 = near
 *   5 = far
 *
 * These primitives are intended for CPU-side culling of chunks and other
 * objects before submitting them to the voxel renderer.
 */
typedef struct
{
    vec4 planes[6];
} Frustum;

/**
 * Extracts the 6 frustum planes from a view-projection matrix.
 *
 * Uses the standard Gribb-Hartmann method. Planes are normalized so that
 * the normal part has unit length.
 *
 * @param vp  [in]  The combined view * projection matrix (column-major, as produced by cglm).
 * @param out [out] The resulting Frustum (must not be null).
 * @retval true  on success.
 * @retval false if the input matrix would produce a degenerate frustum.
 */
b8_t camera_extract_frustum(IN_NONNULL const mat4* vp, OUT_NONNULL Frustum* out) NONNULL_ARGS();

/**
 * Tests whether an axis-aligned bounding box intersects or is contained
 * within the frustum.
 *
 * This is a conservative test (may return true for some boxes that are
 * actually outside due to the nature of AABB vs plane tests).
 *
 * @param frustum [in]  The view frustum.
 * @param min     [in]  Minimum corner of the AABB (in the same space as the frustum planes).
 * @param max     [in]  Maximum corner of the AABB.
 * @retval true if the box may be visible (intersects the frustum volume).
 * @retval false if the box is definitely outside the frustum.
 */
b8_t aabb_in_frustum(IN_NONNULL const Frustum* frustum, vec3 min, vec3 max) NONNULL_ARGS(1);

/**
 * Tests whether a sphere intersects or is contained within the frustum.
 *
 * Useful for rough culling of objects with a bounding sphere.
 *
 * @param frustum [in] The view frustum.
 * @param center  [in] Center of the sphere (same space as planes).
 * @param radius  [in] Radius of the sphere (> 0).
 * @retval true if the sphere may be visible.
 * @retval false if the sphere is definitely outside.
 */
b8_t sphere_in_frustum(IN_NONNULL const Frustum* frustum, const vec3 center, f32_t radius) NONNULL_ARGS(1);

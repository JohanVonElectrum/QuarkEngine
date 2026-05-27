#pragma once

#include <quark/core/camera.h>

#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>

b8_t camera_get_view_projection_matrix(IN_NONNULL const Camera* camera, f32_t aspect_ratio, OUT_NONNULL mat4 out_matrix) NONNULL_ARGS(1, 3);

b8_t camera_extract_frustum(IN_NONNULL const mat4* vp, OUT_NONNULL Frustum* out) NONNULL_ARGS();
b8_t aabb_in_frustum(IN_NONNULL const Frustum* frustum, vec3 min, vec3 max) NONNULL_ARGS(1);
b8_t sphere_in_frustum(IN_NONNULL const Frustum* frustum, const vec3 center, f32_t radius) NONNULL_ARGS(1);

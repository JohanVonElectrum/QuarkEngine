#pragma once

#include <quark/core/camera.h>
#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>

b8_t camera_get_view_projection_matrix(IN_NONNULL const Camera* camera, f32_t aspect_ratio, OUT_NONNULL mat4 out_matrix) NONNULL_ARGS(1, 3);

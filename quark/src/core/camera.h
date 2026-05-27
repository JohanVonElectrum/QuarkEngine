#pragma once

#include <quark/core/camera.h>
#include <cstdlib/primitives.h>

b8_t camera_get_view_projection_matrix(const Camera* camera, f32_t aspect_ratio, mat4 out_matrix);

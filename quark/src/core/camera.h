#pragma once

#include <quark/core/camera.h>

QUARK_B8 camera_get_view_projection_matrix(const Camera* camera, QUARK_F32 aspect_ratio, mat4 out_matrix);

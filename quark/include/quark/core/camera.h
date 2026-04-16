#pragma once

#include <quark/primitives.h>

#include <cglm/types.h>

typedef struct
{
    vec3 position;
    vec3 forward;
    vec3 up;
    QUARK_F32 fov_y_radians;
    QUARK_F32 near_plane;
    QUARK_F32 far_plane;
} Camera;

#pragma once

#include <cstdlib/primitives.h>
#include <cglm/types.h>

/**
 * Simple perspective camera definition.
 *
 * Used to compute view-projection matrices for rendering.
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

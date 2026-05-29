#include "camera.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <cglm/call.h>

static b8_t camera_get_view_matrix(const Camera* in_camera, mat4 out_matrix) {
    const auto camera = (Camera*)in_camera;

    vec3 forward = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 0.0f, 0.0f};
    vec3 side = {0.0f, 0.0f, 0.0f};
    vec3 adjusted_up = {0.0f, 0.0f, 0.0f};

    if (glm_vec3_norm2(camera->forward) <= 0.0f) {
        QUARK_LOG_ERROR("Camera forward vector must not be zero-length");
        return false;
    }
    glm_normalize_to(camera->forward, forward);

    if (glm_vec3_norm2(camera->up) <= 0.0f) {
        QUARK_LOG_ERROR("Camera up vector must not be zero-length");
        return false;
    }
    glm_normalize_to(camera->up, up);

    glm_vec3_cross(forward, up, side);
    if (glm_vec3_norm2(side) <= 0.0f) {
        QUARK_LOG_ERROR("Camera forward and up vectors must not be parallel");
        return false;
    }
    glm_normalize(side);

    glm_vec3_cross(side, forward, adjusted_up);
    glm_normalize(adjusted_up);

    glm_look(camera->position, forward, adjusted_up, out_matrix);

    return true;
}

static b8_t camera_get_perspective_projection_matrix(const Camera* camera, const f32_t aspect_ratio, mat4 out_matrix) {
    if (aspect_ratio <= 0.0f) {
        QUARK_LOG_ERROR("Camera aspect ratio must be greater than zero");
        return false;
    }

    if (camera->fov_y_radians <= 0.0f || camera->fov_y_radians >= CGLM_PI) {
        QUARK_LOG_ERROR("Camera field of view must be within the open interval (0, pi)");
        return false;
    }

    if (camera->near_plane <= 0.0f || camera->far_plane <= camera->near_plane) {
        QUARK_LOG_ERROR("Camera near/far planes must satisfy 0 < near < far");
        return false;
    }

    glm_perspective_rh_no(camera->fov_y_radians, aspect_ratio, camera->near_plane, camera->far_plane, out_matrix);

    out_matrix[1][1] *= -1.0f;

    return true;
}

b8_t camera_get_view_projection_matrix(const Camera* camera, const f32_t aspect_ratio, mat4 out_matrix) {
    mat4 view_matrix;
    mat4 projection_matrix;

    if (!camera_get_view_matrix(camera, view_matrix)) {
        return false;
    }

    if (!camera_get_perspective_projection_matrix(camera, aspect_ratio, projection_matrix)) {
        return false;
    }

    glm_mat4_mul(projection_matrix, view_matrix, out_matrix);
    return true;
}

b8_t camera_extract_frustum(const mat4* vp, Frustum* out)
{
    mat4 m = {
        { (*vp)[0][0], (*vp)[0][1], (*vp)[0][2], (*vp)[0][3] },
        { (*vp)[1][0], (*vp)[1][1], (*vp)[1][2], (*vp)[1][3] },
        { (*vp)[2][0], (*vp)[2][1], (*vp)[2][2], (*vp)[2][3] },
        { (*vp)[3][0], (*vp)[3][1], (*vp)[3][2], (*vp)[3][3] },
    };

    glm_vec4_add(m[3], m[0], out->planes[0]);
    glm_vec4_sub(m[3], m[0], out->planes[1]);
    glm_vec4_add(m[3], m[1], out->planes[2]);
    glm_vec4_sub(m[3], m[1], out->planes[3]);
    glm_vec4_add(m[3], m[2], out->planes[4]);
    glm_vec4_sub(m[3], m[2], out->planes[5]);

    for (int i = 0; i < 6; ++i) {
        vec4 p = {
            out->planes[i][0],
            out->planes[i][1],
            out->planes[i][2],
            0.0f
        };
        const f32_t len = glm_vec3_norm(p);
        if (len > 1e-6f) {
            out->planes[i][0] /= len;
            out->planes[i][1] /= len;
            out->planes[i][2] /= len;
            out->planes[i][3] /= len;
        } else {
            QUARK_LOG_WARN("camera_extract_frustum: degenerate plane %d encountered", i);
            return false;
        }
    }

    return true;
}

b8_t aabb_in_frustum(const Frustum* frustum, vec3 min, vec3 max)
{
    for (int i = 0; i < 6; ++i) {
        const vec4* plane = &frustum->planes[i];

        const vec3 positive_vertex = {
            (plane[0][0] >= 0.0f) ? max[0] : min[0],
            (plane[0][1] >= 0.0f) ? max[1] : min[1],
            (plane[0][2] >= 0.0f) ? max[2] : min[2],
        };

        const f32_t distance = plane[0][0] * positive_vertex[0] +
                         plane[0][1] * positive_vertex[1] +
                         plane[0][2] * positive_vertex[2] +
                         plane[0][3];

        if (distance < 0.0f) {
            return false;
        }
    }

    return true;
}

b8_t sphere_in_frustum(const Frustum* frustum, const vec3 center, f32_t radius)
{
    if (radius < 0.0f) {
        radius = 0.0f;
    }

    for (int i = 0; i < 6; ++i) {
        const vec4* plane = &frustum->planes[i];

        const f32_t distance = plane[0][0] * center[0] +
                         plane[0][1] * center[1] +
                         plane[0][2] * center[2] +
                         plane[0][3];

        if (distance < -radius) {
            return false;
        }
    }

    return true;
}

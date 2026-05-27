#include "camera.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <cglm/call.h>

static b8_t camera_get_view_matrix(const Camera* in_camera, mat4 out_matrix) {
    QUARK_ASSERT_RETURN(false, in_camera != nullptr && out_matrix != nullptr, "Invalid camera or output matrix pointer");
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
    QUARK_ASSERT_RETURN(false, camera != nullptr && out_matrix != nullptr, "Invalid camera or output matrix pointer");

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

    return true;
}

b8_t camera_get_view_projection_matrix(const Camera* camera, const f32_t aspect_ratio, mat4 out_matrix) {
    QUARK_ASSERT_RETURN(false, camera != nullptr && out_matrix != nullptr, "Invalid camera or output matrix pointer");

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

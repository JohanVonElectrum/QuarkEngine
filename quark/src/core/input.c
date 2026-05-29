#include "input.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <cglm/call.h>
#include <GLFW/glfw3.h>

static GLFWwindow* s_window = nullptr;
static f64_t s_last_mouse_x = 0.0;
static f64_t s_last_mouse_y = 0.0;
static b8_t s_mouse_initialized = false;
static f32_t s_yaw = 0.0f;
static f32_t s_pitch = 0.0f;
static b8_t s_look_initialized = false;
static b8_t s_prev_escape_down = false;

b8_t init_input(QuarkWindow* window) {
    QUARK_ASSERT_RETURN(
        false,
        window,
        "Attempted to initialize input with NULL window"
    );

    s_window = window_get_glfw_handle(window);
    QUARK_ASSERT_RETURN(
        false,
        s_window,
        "Window has no underlying GLFW handle"
    );

    glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwGetCursorPos(s_window, &s_last_mouse_x, &s_last_mouse_y);
    s_mouse_initialized = true;

    s_look_initialized = false;
    s_prev_escape_down = false;

    QUARK_LOG_INFO("Input system initialized (cursor captured)");
    return true;
}

void shutdown_input(void) {
    if (s_window) {
        glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        s_window = nullptr;
    }
    s_mouse_initialized = false;
    s_look_initialized = false;
}

void input_process(const f32_t dt, Camera* camera) {
    QUARK_ASSERT_X(
        { return; },
        camera,
        "Attempted to process input for NULL camera"
    );

    if (!s_window || dt <= 0.0f) {
        return;
    }

    const int focused = glfwGetWindowAttrib(s_window, GLFW_FOCUSED);
    if (!focused) {
        glfwGetCursorPos(s_window, &s_last_mouse_x, &s_last_mouse_y);
        s_mouse_initialized = true;
        return;
    }

    f64_t mx, my;
    glfwGetCursorPos(s_window, &mx, &my);

    f32_t mouse_dx = 0.0f;
    f32_t mouse_dy = 0.0f;
    if (s_mouse_initialized) {
        mouse_dx = (f32_t)(mx - s_last_mouse_x);
        mouse_dy = (f32_t)(my - s_last_mouse_y);
    } else {
        s_mouse_initialized = true;
    }
    s_last_mouse_x = mx;
    s_last_mouse_y = my;

    const int esc_state = glfwGetKey(s_window, GLFW_KEY_ESCAPE);
    const b8_t escape_down = (esc_state == GLFW_PRESS);
    if (escape_down && !s_prev_escape_down) {
        const int mode = glfwGetInputMode(s_window, GLFW_CURSOR);
        if (mode == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(s_window, &s_last_mouse_x, &s_last_mouse_y);
            s_mouse_initialized = true;
        }
    }
    s_prev_escape_down = escape_down;

    const int cursor_mode = glfwGetInputMode(s_window, GLFW_CURSOR);
    const b8_t apply_look = (cursor_mode == GLFW_CURSOR_DISABLED);

    if (!s_look_initialized) {
        vec3 fwd;
        glm_normalize_to(camera->forward, fwd);

        s_yaw = atan2f(fwd[0], fwd[2]);
        s_pitch = asinf(fwd[1]);

        s_look_initialized = true;
    }

    if (apply_look) {
        constexpr f32_t MOUSE_SENSITIVITY = 0.0022f;
        constexpr f32_t PITCH_LIMIT = 1.553343f; // ~89 deg

        s_yaw -= mouse_dx * MOUSE_SENSITIVITY;
        s_pitch -= mouse_dy * MOUSE_SENSITIVITY;

        if (s_pitch > PITCH_LIMIT) s_pitch = PITCH_LIMIT;
        if (s_pitch < -PITCH_LIMIT) s_pitch = -PITCH_LIMIT;

        const f32_t cp = cosf(s_pitch);
        camera->forward[0] = sinf(s_yaw) * cp;
        camera->forward[1] = sinf(s_pitch);
        camera->forward[2] = cosf(s_yaw) * cp;

        glm_normalize(camera->forward);
    }

    vec3 local_move = {0.0f, 0.0f, 0.0f};
    b8_t vertical_key = false;

    if (glfwGetKey(s_window, GLFW_KEY_W) == GLFW_PRESS) local_move[2] -= 1.0f;
    if (glfwGetKey(s_window, GLFW_KEY_S) == GLFW_PRESS) local_move[2] += 1.0f;
    if (glfwGetKey(s_window, GLFW_KEY_A) == GLFW_PRESS) local_move[0] -= 1.0f;
    if (glfwGetKey(s_window, GLFW_KEY_D) == GLFW_PRESS) local_move[0] += 1.0f;

    f32_t vertical = 0.0f;
    if (glfwGetKey(s_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        vertical += 1.0f;
        vertical_key = true;
    }
    if (glfwGetKey(s_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(s_window, GLFW_KEY_C) == GLFW_PRESS) {
        vertical -= 1.0f;
        vertical_key = true;
    }

    const f32_t horiz_len2 = local_move[0]*local_move[0] + local_move[2]*local_move[2];
    const b8_t any_horizontal = (horiz_len2 > 1e-4f);

    if (any_horizontal || vertical_key) {
        vec3 fwd, right, world_up = {0.0f, 1.0f, 0.0f};

        glm_normalize_to(camera->forward, fwd);

        glm_vec3_cross(fwd, world_up, right);
        if (glm_vec3_norm2(right) < 1e-4f) {
            vec3 alt = {1.0f, 0.0f, 0.0f};
            glm_vec3_cross(fwd, alt, right);
        }
        glm_normalize(right);

        f32_t speed = 5.0f;
        if (glfwGetKey(s_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            speed *= 4.0f;
        }

        vec3 velocity = {0.0f, 0.0f, 0.0f};

        if (any_horizontal) {
            vec3 horiz;
            glm_vec3_scale(right, local_move[0], horiz);
            vec3 fwd_h;
            glm_vec3_scale(fwd, -local_move[2], fwd_h);
            glm_vec3_add(horiz, fwd_h, horiz);

            glm_vec3_scale(horiz, speed * dt, horiz);
            glm_vec3_add(velocity, horiz, velocity);
        }

        if (vertical_key) {
            vec3 vert_move;
            glm_vec3_scale(world_up, vertical * speed * dt, vert_move);
            glm_vec3_add(velocity, vert_move, velocity);
        }

        glm_vec3_add(camera->position, velocity, camera->position);
    }
}

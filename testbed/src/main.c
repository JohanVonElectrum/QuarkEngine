#include <quark/entrypoint.h>

b8_t init_application(ApplicationCreateInfo* create_info)
{
    create_info->name = "Testbed";
    create_info->version = (Version){
        .major = 0,
        .minor = 1,
        .patch = 0,
    };
    create_info->camera = (Camera){
        .position = {2.0f, 3.0f, 4.0f},
        .forward = {-2.0f, -3.0f, -4.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fov_y_radians = 1.0471976f,
        .near_plane = 0.1f,
        .far_plane = 100.0f,
    };
    create_info->window = (WindowCreateInfo){
        .mode = WINDOW_MODE_WINDOWED,
        .data = {
            .graphics = {
                .title = "Testbed",
                .width = 1280,
                .height = 720,
            }
        }
    };

    QUARK_LOG_INFO("Initialized application create info");

    return true;
}

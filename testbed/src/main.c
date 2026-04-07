#include <quark/entrypoint.h>

QUARK_B8 init_application(ApplicationCreateInfo* create_info)
{
    create_info->name = "Testbed";
    create_info->version = (Version){
        .major = 0,
        .minor = 1,
        .patch = 0,
    };
    create_info->window = (WindowCreateInfo){
        .mode = GRAPHICS_MODE_VULKAN,
        .data = {
            .graphics = {
                .title = "Testbed",
                .width = 1280,
                .height = 720,
            }
        }
    };

    QUARK_LOG_INFO("Initialized application create info");

    return QUARK_TRUE;
}

#pragma once

#include <quark/core/application.h>
#include <quark/core/log.h>

int main(void) {
    QUARK_LOG_INFO("Starting Quark");

    ApplicationCreateInfo create_info;
    if (!init_application(&create_info)) {
        QUARK_LOG_ERROR("Failed to initialize application");
        return -1;
    }

    QUARK_LOG_INFO("Loading \"%s\"...", create_info.name);

    Application* application = create_application(&create_info);
    if (!application) {
        QUARK_LOG_ERROR("Failed to create application");
        return -2;
    }

    const QUARK_B8 success = run_application(application);
    if (!success) {
        QUARK_LOG_ERROR("Application exited with an error");
    }

    if (!destroy_application(application)) {
        QUARK_LOG_ERROR("Failed to destroy application");
        return -4;
    }

    return success ? 0 : -3;
}

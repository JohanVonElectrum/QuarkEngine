#pragma once

#include <quark/core/application.h>
#include <quark/core/engine.h>
#include <quark/core/log.h>

#undef QUARK_LOG_INTERNAL_ENGINE
#define QUARK_LOG_INTERNAL_ENGINE QUARK_TRUE

int main(const int argc, char** argv) {
    if (!init_quark(argc, argv)) {
        QUARK_LOG_FATAL("Failed to initialize Quark");
        return QUARK_EXIT_CODE_FAILED_TO_INITIALIZE_ENGINE;
    }

    int exit_code = QUARK_EXIT_CODE_SUCCESS;

    ApplicationCreateInfo create_info;
    if (!init_application(&create_info)) {
        QUARK_LOG_FATAL("Failed to initialize application");
        exit_code |= QUARK_EXIT_CODE_FAILED_TO_INITIALIZE_APPLICATION;
        goto engine_shutdown;
    }

    QUARK_LOG_INFO("Loading \"%s\"...", create_info.name);

    Application* application = create_application(&create_info);
    if (!application) {
        QUARK_LOG_FATAL("Failed to create application");
        exit_code |= QUARK_EXIT_CODE_FAILED_TO_CREATE_APPLICATION;
        goto engine_shutdown;
    }

    if (!run_application(application)) {
        QUARK_LOG_ERROR("Application exited with an error");
        exit_code |= QUARK_EXIT_CODE_FAILED_TO_RUN_APPLICATION;
    }

    if (!destroy_application(application)) {
        QUARK_LOG_FATAL("Failed to destroy application");
        exit_code |= QUARK_EXIT_CODE_FAILED_TO_DESTROY_APPLICATION;
    }

engine_shutdown:
    if (!shutdown_quark()) {
        QUARK_LOG_FATAL("Failed to shutdown Quark");
        exit_code |= QUARK_EXIT_CODE_FAILED_TO_SHUTDOWN_ENGINE;
    }

    return exit_code;
}

#undef QUARK_LOG_INTERNAL_ENGINE
#define QUARK_LOG_INTERNAL_ENGINE QUARK_FALSE

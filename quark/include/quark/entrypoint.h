#pragma once

#include <quark/core/application.h>

int main(void) {
    ApplicationCreateInfo create_info;
    init_application(&create_info);

    Application* application = create_application(&create_info);
    if (!application) {
        return -1;
    }

    const QUARK_B8 success = run_application(application);
    if (!success) {
        // TODO: Log error
    }

    if (!destroy_application(application)) {
        return -3;
    }

    return success ? 0 : -2;
}

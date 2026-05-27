#pragma once

#include <quark/api.h>

#include <cstdlib/common.h>

/**
 * Exit codes that can be returned from the engine's main entry point.
 *
 * These are bit flags so multiple failure reasons can be reported.
 */
enum QUARK_EXIT_CODE
{
    QUARK_EXIT_CODE_SUCCESS = 0,
    QUARK_EXIT_CODE_FAILED_TO_INITIALIZE_ENGINE = BIT(32 - 1),
    QUARK_EXIT_CODE_FAILED_TO_INITIALIZE_APPLICATION = BIT(32 - 2),
    QUARK_EXIT_CODE_FAILED_TO_CREATE_APPLICATION = BIT(32 - 3),
    QUARK_EXIT_CODE_FAILED_TO_RUN_APPLICATION = BIT(32 - 4),
    QUARK_EXIT_CODE_FAILED_TO_DESTROY_APPLICATION = BIT(32 - 5),
    QUARK_EXIT_CODE_FAILED_TO_SHUTDOWN_ENGINE = BIT(32 - 6),
};

/**
 * Initialize the Quark engine.
 *
 * Must be called before any other Quark API (except possibly logging in very
 * early failure paths). This initializes cstdlib, the tracing subsystem,
 * and other core facilities.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings (may be NULL if argc is 0).
 * @retval true on success.
 * @retval false on failure (the engine is not usable after this point).
 *
 * @see shutdown_quark
 */
QUARK_EXPORT b8_t init_quark(int argc, IN_NULLABLE char** argv);

/**
 * Shut down the Quark engine.
 *
 * Should be called during final cleanup. This shuts down the tracing worker
 * and releases any global resources.
 *
 * @retval true on success.
 * @retval false on failure during shutdown.
 *
 * @see init_quark
 */
QUARK_EXPORT b8_t shutdown_quark(void);

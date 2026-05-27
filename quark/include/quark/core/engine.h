#pragma once

#include <cstdlib/common.h>
#include <quark/api.h>

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
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @retval true on success.
 * @retval false on failure (engine is not usable).
 */
QUARK_EXPORT b8_t init_quark(int argc, char** argv);

/**
 * Shut down the Quark engine.
 *
 * Should be called during final cleanup. This shuts down the tracing worker
 * and releases any global resources.
 *
 * @retval true on success.
 * @retval false on failure during shutdown.
 */
QUARK_EXPORT b8_t shutdown_quark(void);

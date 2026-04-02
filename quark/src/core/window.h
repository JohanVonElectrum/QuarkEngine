#pragma once

#include <quark/core/window.h>

QUARK_B8 init_windowing();
QUARK_B8 shutdown_windowing();
QUARK_B8 windowing_poll_events();

typedef struct QuarkWindow QuarkWindow;

QuarkWindow* create_window(const WindowCreateInfo* create_info);
QUARK_B8 destroy_window(QuarkWindow* window);
QUARK_B8 window_should_close(const QuarkWindow* window);

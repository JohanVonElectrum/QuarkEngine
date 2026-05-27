#pragma once

#include <quark/core/window.h>
#include <cstdlib/common.h>

b8_t init_windowing();
b8_t shutdown_windowing();
b8_t windowing_poll_events();

typedef struct QuarkWindow QuarkWindow;

QuarkWindow* create_window(const WindowCreateInfo* create_info);
b8_t destroy_window(QuarkWindow* window);
b8_t window_should_close(const QuarkWindow* window);

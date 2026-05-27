#pragma once

#include "vk.h"

#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>

b8_t create_vulkan_device(IN_NONNULL VulkanContext* context) NONNULL_ARGS();
b8_t destroy_vulkan_device(IN_NONNULL VulkanContext* context) NONNULL_ARGS();

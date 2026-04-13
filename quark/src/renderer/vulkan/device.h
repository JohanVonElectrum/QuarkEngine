#pragma once

#include "vk.h"

#include <quark/primitives.h>

QUARK_B8 create_vulkan_device(VulkanContext* context);
QUARK_B8 destroy_vulkan_device(VulkanContext* context);

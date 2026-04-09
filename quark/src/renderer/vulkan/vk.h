#pragma once

#include <quark/macro.h>
#include <quark/core/assert.h>

#include <vulkan/vulkan.h>

#define VK_CHECK_X(expr, extra) { \
    const VkResult result = (expr); \
    QUARK_INTERNAL_ASSERT_IMPL(extra, result == VK_SUCCESS, "Failed to execute `%s` with result `%u`", QUARK_STRINGIFY_MACRO(expr), result); \
}
#define VK_CHECK(expr) VK_CHECK_X(expr, {})
#define VK_CHECK_RETURN(expr, ret) VK_CHECK_X(expr, { return ret; })

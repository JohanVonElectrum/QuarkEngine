#pragma once

#define QUARK_EXPAND_MACRO(x) x
#define QUARK_STRINGIFY_MACRO(x) #x
#define QUARK_MERGE_HELPER(a, b) a##b
#define QUARK_MERGE(a, b) QUARK_MERGE_HELPER(a, b)

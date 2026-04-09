#pragma once

#include <quark/primitives.h>

void* quark_mem_alloc(QUARK_USIZE size);
void* quark_mem_calloc(QUARK_USIZE count, QUARK_USIZE stride);
void* quark_mem_realloc(void* ptr, QUARK_USIZE size);
QUARK_B8 quark_mem_free(void* ptr);
void* quark_mem_reserve(QUARK_USIZE size);
void* quark_mem_commit(void* ptr, QUARK_USIZE size);
QUARK_B8 quark_mem_decommit(void* ptr, QUARK_USIZE size);
QUARK_B8 quark_mem_release(void* ptr, QUARK_USIZE size);

void quark_mem_copy(void* dest, const void* src, QUARK_USIZE size);

QUARK_USIZE quark_mem_alignment();
QUARK_USIZE quark_mem_page_size();

#define QUARK_ALIGN_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))
#define QUARK_ALIGN_DOWN(value, alignment) ((value) & ~((alignment) - 1))


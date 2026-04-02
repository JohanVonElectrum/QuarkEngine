#pragma once

#include <stddef.h>

#define QUARK_U8 unsigned char
#define QUARK_U16 unsigned short
#define QUARK_U32 unsigned int
#define QUARK_U64 unsigned long long
#define QUARK_USIZE size_t
#define QUARK_UPTR uintptr_t
#define QUARK_I8 char
#define QUARK_I16 short
#define QUARK_I32 int
#define QUARK_I64 long long
#define QUARK_ISIZE ssize_t
#define QUARK_IPTR intptr_t
#define QUARK_F32 float
#define QUARK_F64 double
#define QUARK_B8 unsigned char
#define QUARK_B32 unsigned int

#define QUARK_FALSE 0
#define QUARK_TRUE 1

#define BIT(x) (1 << (x))

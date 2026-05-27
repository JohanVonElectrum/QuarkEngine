# Code Conventions

## General Language and Style

- C23 is required (`CMAKE_C_STANDARD 23`). `nullptr`, `thread_local`, `constexpr`, and designated initializers are used throughout.
- All heap memory must be obtained through cstdlib (`<cstdlib/mem.h>`): `mem_heap_alloc`, `mem_heap_calloc`, `mem_heap_free`, `mem_copy`, etc. Direct use of `malloc`/`free` is forbidden.
- `cstdlib_init` is called automatically inside `init_quark`; user code should not call it directly.

## Include Ordering (Strictly Enforced)

- Quote includes (`"..."`) are reserved **exclusively** for Quark internal headers (never for cstdlib, GLFW, Vulkan, cglm, etc.).
- In `.c` files: the very first line **must** be the quote include for the header being implemented (e.g. `#include "backend.h"`), followed by a blank line.
- After one blank line: all `"quark/..."` internal headers (grouped together).
- After another blank line: all `<quark/...>` public headers (grouped together).
- After another blank line: third-party library headers (`<cstdlib/...>`, `<GLFW/...>`, `<vulkan/...>`, `<cglm/...>`, etc.).
- After yet another blank line: standard C headers (`<stdio.h>`, `<string.h>`, `<stddef.h>`, `<stdatomic.h>`, `<stdarg.h>`, etc.).
- The same grouping order applies to `.h` files (without the requirement that the first line is a quote include).

## Nullability Annotation Rules

Using macros from `<cstdlib/nullability.h>`:

- Whenever `IN_NONNULL`, `OUT_NONNULL`, `INOUT_NONNULL`, etc. is used on **at least one** function parameter or return value, the function **must** also be annotated with `NONNULL_ARGS(...)`.
- If **every** argument of the function is a non-null pointer, use the form with no arguments: `NONNULL_ARGS()`.
- If the function has any non-pointer arguments or any pointers that are allowed to be null, explicitly list the 1-based argument indices: `NONNULL_ARGS(1, 3)`.
- Example:
  ```c
  void foo(IN_NONNULL Bar* a, int b, OUT_NONNULL Baz* c) NONNULL_ARGS(1, 3);
  void bar(IN_NONNULL void* p) NONNULL_ARGS();   // all args are NONNULL pointers
  ```

## Other Conventions

- Guard-clause error handling uses the `QUARK_ASSERT`, `QUARK_ASSERT_RETURN`, and `QUARK_ASSERT_X` family from `quark/include/quark/core/assert.h`. The `_X` variant accepts a block that is executed on failure before returning (commonly used for cleanup).
- Logging (`QUARK_LOG_FATAL` … `QUARK_LOG_TRACE`) and tracing spans are always asynchronous. Messages and span data are posted to a lock-free event queue consumed by a dedicated worker thread. The implementation lives in `quark/src/core/tracing.c` and `quark/src/core/event.c` and depends on cstdlib thread and clock facilities. Output is currently console only.
- Vulkan calls must be wrapped with the `VK_CHECK`, `VK_CHECK_RETURN`, or `VK_CHECK_X` macros defined in `quark/src/renderer/vk.h`; these macros integrate with the Quark assertion system.

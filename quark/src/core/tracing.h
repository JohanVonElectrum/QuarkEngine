#pragma once

#include <quark/core/log.h>

QUARK_B8 init_tracing();
QUARK_B8 shutdown_tracing();

typedef struct QuarkTracingSpan QuarkTracingSpan;

const QuarkTracingSpan* quark_start_tracing_span();
void quark_end_tracing_span(const QuarkTracingSpan* span);

#define QUARK_TRACE_SPAN(x) {\
    const QuarkTracingSpan* quark_tracing_span = quark_start_tracing_span();\
    {x};\
    quark_end_tracing_span(quark_tracing_span);\
}

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TRACE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TRACE_CAPI_H_

#include <stdint.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// [Internal] Begins a trace section.
// This function is intended to be used via the LYNX_EXTENSION_TRACE_BEGIN
// macro.
LYNX_CAPI_EXPORT void lynx_trace_section_begin(const char* category,
                                               const char* name);

// [Internal] Ends a trace section.
// This function is intended to be used via the LYNX_EXTENSION_TRACE_END macro.
LYNX_CAPI_EXPORT void lynx_trace_section_end(const char* category,
                                             const char* name);

// [Internal] Records an instant event.
// This function is intended to be used via the LYNX_EXTENSION_TRACE_INSTANT
// macro.
LYNX_CAPI_EXPORT void lynx_trace_instant(const char* category,
                                         const char* name);

// [Internal] Records a counter event.
LYNX_CAPI_EXPORT void lynx_trace_counter(const char* category, const char* name,
                                         uint64_t value, bool incremental);

LYNX_EXTERN_C_END

// Begins a trace section via C-API.
// Must be paired with LYNX_CAPI_TRACE_END.
// Example:
//   void my_traced_function() {
//     LYNX_CAPI_TRACE_BEGIN("MyCategory", "my_traced_function");
//     // ... code to be traced ...
//     LYNX_CAPI_TRACE_END("MyCategory", "my_traced_function");
//   }
#define LYNX_CAPI_TRACE_BEGIN(category, name) \
  lynx_trace_section_begin(category, name)

// Ends a trace section via C-API.
#define LYNX_CAPI_TRACE_END(category, name) \
  lynx_trace_section_end(category, name)

// Records an instant event via C-API.
// Example: LYNX_CAPI_TRACE_INSTANT("MyCategory", "ButtonClicked");
#define LYNX_CAPI_TRACE_INSTANT(category, name) \
  lynx_trace_instant(category, name)

// Records a counter event via C-API.
// Example: LYNX_CAPI_TRACE_COUNTER("MyCategory", "MyCounter", 42, false);
#define LYNX_CAPI_TRACE_COUNTER(category, name, value, incremental) \
  lynx_trace_counter(category, name, value, incremental)

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TRACE_CAPI_H_

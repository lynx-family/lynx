// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_FSP_TRACE_EVENT_DEF_H_
#define CORE_SERVICES_FSP_TRACING_FSP_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

// Category definitions
inline constexpr const char* const LYNX_TRACE_CATEGORY_FSP =
    "disabled-by-default-devtools.fsp";

// Event name definitions for LynxFSPTracer
inline constexpr const char* const FSP_LYNX_TRACER_CREATE =
    "LynxFSPTracer.create";
inline constexpr const char* const FSP_LYNX_TRACER_BEGIN_TRACING =
    "LynxFSPTracer.beginTracing";
inline constexpr const char* const FSP_LYNX_TRACER_STOP_TRACING =
    "LynxFSPTracer.stopTracing";
inline constexpr const char* const FSP_LYNX_TRACER_CAPTURE_SNAPSHOT =
    "LynxFSPTracer.captureSnapshot";
inline constexpr const char* const FSP_LYNX_TRACER_UPDATE_SNAPSHOT =
    "LynxFSPTracer.updateSnapshot";

// Event name definitions for C++ FSPTracer
inline constexpr const char* const FSP_TRACER_CAPTURE_SNAPSHOT =
    "FSPTracer::CaptureSnapshot";
inline constexpr const char* const FSP_TRACER_FILL_SNAPSHOT =
    "FSPTracer::FillSnapshot";
inline constexpr const char* const FSP_TRACER_INSPECT_SNAPSHOT =
    "FSPTracer::InspectSnapshot";

// Debug annotation keys
inline constexpr const char* const FSP_DEBUG_SIZE = "size";
inline constexpr const char* const FSP_DEBUG_RECT = "rect";
inline constexpr const char* const FSP_DEBUG_UI_SIGN = "ui_sign";
inline constexpr const char* const FSP_DEBUG_STABLE = "stable";
inline constexpr const char* const FSP_DEBUG_REASON = "reason";
inline constexpr const char* const FSP_DEBUG_TIMESTAMP = "timestamp";
inline constexpr const char* const FSP_DEBUG_TOTAL_COUNT = "total_cnt";
inline constexpr const char* const FSP_DEBUG_LOADED_COUNT = "loaded_cnt";
inline constexpr const char* const FSP_DEBUG_LOADING_COUNT = "loaded_cnt";

#endif  // ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_SERVICES_FSP_TRACING_FSP_TRACE_EVENT_DEF_H_

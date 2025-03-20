// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EVENT_MOCK_H_
#define BASE_TRACE_NATIVE_TRACE_EVENT_MOCK_H_

#define TRACE_EVENT_BEGIN(category, name, ...)
#define TRACE_EVENT_END(category, ...)

#define TRACE_EVENT(category, name, ...)
#define TRACE_EVENT_INSTANT(category, name, ...)
#define TRACE_EVENT_CATEGORY_ENABLED(category)
#define TRACE_COUNTER(category, track, ...)
#define TRACE_FLOW_ID() 0

#endif  // BASE_TRACE_NATIVE_TRACE_EVENT_MOCK_H_

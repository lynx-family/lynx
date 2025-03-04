// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/trace/native/trace_event_utils_systrace.h"

namespace lynx {
namespace trace {

void InitSystraceBeginSection(ATrace_beginSection_ptr atrace_beginsection) {}
void InitSystraceEndSection(ATrace_endSection_ptr atrace_endsection) {}
void InitSystraceBeginAsynSection(
    ATrace_beginAsyncSection_ptr atrace_beginasyncsection) {}
void InitSystraceEndAsynSection(
    ATrace_endAsyncSection_ptr atrace_endasyncsection) {}

void TraceEventBegin(const char* name) {}
void TraceEventBegin(const char* name, uint64_t cookie) {}
void TraceEventBegin(const std::string& name) {}
void TraceEventBegin(const std::string& name, uint64_t cookie) {}
void TraceEventEnd() {}
void TraceEventEnd(const char* name, uint64_t cookie) {}

}  // namespace trace
}  // namespace lynx

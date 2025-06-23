// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/trace/native/trace_event_utils_systrace.h"
#include "hitrace/trace.h"

namespace lynx {
namespace trace {

void InitSystraceBeginSection(ATrace_beginSection_ptr atrace_beginsection) {}
void InitSystraceEndSection(ATrace_endSection_ptr atrace_endsection) {}
void InitSystraceBeginAsynSection(
    ATrace_beginAsyncSection_ptr atrace_beginasyncsection) {}
void InitSystraceEndAsynSection(
    ATrace_endAsyncSection_ptr atrace_endasyncsection) {}

void TraceEventBegin(const char* name) { OH_HiTrace_StartTrace(name); }
void TraceEventBegin(const char* name,
                     __attribute__((unused)) uint64_t cookie) {
  OH_HiTrace_StartTrace(name);
}
void TraceEventBegin(const std::string& name) {
  OH_HiTrace_StartTrace(name.c_str());
}
void TraceEventBegin(const std::string& name,
                     __attribute__((unused)) uint64_t cookie) {
  OH_HiTrace_StartTrace(name.c_str());
}
void TraceEventEnd() { OH_HiTrace_FinishTrace(); }
void TraceEventEnd(__attribute__((unused)) const char* name,
                   __attribute__((unused)) uint64_t cookie) {
  OH_HiTrace_FinishTrace();
}

}  // namespace trace
}  // namespace lynx

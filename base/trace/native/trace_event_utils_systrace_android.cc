// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/trace/native/trace_event_utils_systrace.h"

namespace lynx {
namespace trace {
namespace {

static ATrace_beginSection_ptr ATrace_beginSection = nullptr;
static ATrace_endSection_ptr ATrace_endSection = nullptr;
static ATrace_beginAsyncSection_ptr ATrace_beginAsyncSection = nullptr;
static ATrace_endAsyncSection_ptr ATrace_endAsyncSection = nullptr;

}  // namespace

void InitSystraceBeginSection(ATrace_beginSection_ptr atrace_beginsection) {
  ATrace_beginSection = atrace_beginsection;
}
void InitSystraceEndSection(ATrace_endSection_ptr atrace_endsection) {
  ATrace_endSection = atrace_endsection;
}
void InitSystraceBeginAsynSection(
    ATrace_beginAsyncSection_ptr atrace_beginasyncsection) {
  ATrace_beginAsyncSection = atrace_beginasyncsection;
}
void InitSystraceEndAsynSection(
    ATrace_endAsyncSection_ptr atrace_endasyncsection) {
  ATrace_endAsyncSection = atrace_endasyncsection;
}

void TraceEventBegin(const char* name) {
  if (ATrace_beginSection) {
    ATrace_beginSection(name);
  }
}
void TraceEventBegin(const char* name, uint64_t cookie) {
  if (ATrace_beginAsyncSection) {
    ATrace_beginAsyncSection(name, int32_t(cookie));
  }
}
void TraceEventBegin(const std::string& name) { TraceEventBegin(name.c_str()); }
void TraceEventBegin(const std::string& name, uint64_t cookie) {
  TraceEventBegin(name.c_str(), cookie);
}
void TraceEventEnd() {
  if (ATrace_endSection) {
    ATrace_endSection();
  }
}
void TraceEventEnd(const char* name, uint64_t cookie) {
  if (ATrace_endAsyncSection) {
    ATrace_endAsyncSection(name, int32_t(cookie));
  }
}

}  // namespace trace
}  // namespace lynx

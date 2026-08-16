// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdio>
#include <cstring>
#include <memory>

#include "base/trace/native/trace_event.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "quickjs/include/quickjs.h"

namespace lynx {
namespace runtime {
namespace js {

namespace {

class QuickjsHeapSnapshot final : public HeapSnapshot {
 public:
  explicit QuickjsHeapSnapshot(const char* snapshot) : snapshot_(snapshot) {}
  ~QuickjsHeapSnapshot() override { js_profile_free_heap_snapshot(snapshot_); }

  bool WriteToFile(const std::string& output_path) const override {
    if (snapshot_ == nullptr || output_path.empty()) {
      return false;
    }

    FILE* output = std::fopen(output_path.c_str(), "wb");
    if (output == nullptr) {
      return false;
    }

    const size_t snapshot_length = std::strlen(snapshot_);
    const bool success = std::fwrite(snapshot_, sizeof(char), snapshot_length,
                                     output) == snapshot_length;
    return std::fclose(output) == 0 && success;
  }

 private:
  const char* snapshot_;
};

}  // namespace

std::unique_ptr<HeapSnapshot> QuickjsRuntime::TakeHeapSnapshot() {
  LEPUSContext* context = getJSContext();
  if (context == nullptr) {
    return nullptr;
  }

  const char* snapshot = js_profile_take_heap_snapshot(context);
  if (snapshot == nullptr) {
    return nullptr;
  }
  return std::make_unique<QuickjsHeapSnapshot>(snapshot);
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx

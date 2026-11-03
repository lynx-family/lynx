// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "quickjs/include/quickjs.h"

namespace lynx {
namespace runtime {
namespace js {

namespace {

class QuickjsHeapSnapshot final : public CStringRefBuffer {
 public:
  explicit QuickjsHeapSnapshot(const char* snapshot)
      : CStringRefBuffer(snapshot) {}
  ~QuickjsHeapSnapshot() override {
    js_profile_free_heap_snapshot(reinterpret_cast<const char*>(data()));
  }

  QuickjsHeapSnapshot(const QuickjsHeapSnapshot&) = delete;
  QuickjsHeapSnapshot& operator=(const QuickjsHeapSnapshot&) = delete;
};

}  // namespace

std::unique_ptr<Buffer> QuickjsRuntime::TakeHeapSnapshot() {
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

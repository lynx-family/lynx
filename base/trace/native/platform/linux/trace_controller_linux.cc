// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "base/trace/native/trace_controller.h"

namespace lynx::trace {

namespace {

class TraceControllerDelegateLinux : public TraceController::Delegate {
 public:
  TraceControllerDelegateLinux() = default;
  ~TraceControllerDelegateLinux() override = default;

  std::string GenerateTracingFileDir() override {
    const char* home_dir = std::getenv("HOME");
    if (home_dir && home_dir[0] != '\0') {
      return home_dir;
    }
    return "/tmp";
  }
};

}  // namespace

TraceController* GetTraceControllerInstance() {
  static bool should_init_delegate = true;
  if (should_init_delegate) {
    auto delegate = std::make_unique<TraceControllerDelegateLinux>();
    TraceController::Instance()->SetDelegate(std::move(delegate));
    should_init_delegate = false;
  }
  return TraceController::Instance();
}

}  // namespace lynx::trace

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_PLATFORM_DARWIN_TRACE_CONTROLLER_DARWIN_H_
#define BASE_TRACE_NATIVE_PLATFORM_DARWIN_TRACE_CONTROLLER_DARWIN_H_

#include <string>

#include "base/trace/native/trace_controller.h"
#include "base/trace/native/trace_export.h"

namespace lynx {
namespace trace {

class TRACE_EXPORT TraceControllerDelegateDarwin
    : public TraceController::Delegate {
 public:
  TraceControllerDelegateDarwin() = default;
  virtual ~TraceControllerDelegateDarwin() = default;

  std::string GenerateTracingFileDir() override;

 private:
  TraceControllerDelegateDarwin(const TraceControllerDelegateDarwin&) = delete;
  TraceControllerDelegateDarwin& operator=(
      const TraceControllerDelegateDarwin&) = delete;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_PLATFORM_DARWIN_TRACE_CONTROLLER_DARWIN_H_

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_PLATFORM_HARMONY_TRACE_CONTROLLER_DELEGATE_HARMONY_H_
#define BASE_TRACE_NATIVE_PLATFORM_HARMONY_TRACE_CONTROLLER_DELEGATE_HARMONY_H_

#include <memory>
#include <string>

#include "base/trace/native/trace_controller.h"
#include "base/trace/native/trace_export.h"

namespace lynx {
namespace trace {

class TRACE_EXPORT TraceControllerDelegateHarmony
    : public TraceController::Delegate {
 public:
  TraceControllerDelegateHarmony() = default;
  ~TraceControllerDelegateHarmony() = default;

  std::string GenerateTracingFileDir() override;
  static void SetTraceDirPath(const std::string& dir);

 private:
  static std::string trace_dir_path_;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_PLATFORM_HARMONY_TRACE_CONTROLLER_DELEGATE_HARMONY_H_

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_NATIVE_FACADE_REPORTER_H_
#define CORE_SHELL_NATIVE_FACADE_REPORTER_H_

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace shell {

class NativeFacadeReporter {
 public:
  NativeFacadeReporter() = default;

  virtual ~NativeFacadeReporter() = default;

  NativeFacadeReporter(const NativeFacadeReporter &facade) = delete;

  NativeFacadeReporter &operator=(const NativeFacadeReporter &) = delete;

  NativeFacadeReporter(NativeFacadeReporter &&facade) = default;

  NativeFacadeReporter &operator=(NativeFacadeReporter &&) = default;

  virtual void OnPerformanceEvent(const lepus::Value &entry) = 0;

};  // class NativeFacadeReporter

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_NATIVE_FACADE_REPORTER_H_

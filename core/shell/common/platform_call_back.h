// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_COMMON_PLATFORM_CALL_BACK_H_
#define CORE_SHELL_COMMON_PLATFORM_CALL_BACK_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "base/include/closure.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace shell {

class PlatformCallBack {
 public:
  using DataCallBackType = std::function<void(const lepus::Value&)>;

  PlatformCallBack(DataCallBackType func = nullptr);
  virtual ~PlatformCallBack() = default;

  int32_t id() { return id_; }
  // Move Only.
  PlatformCallBack(const PlatformCallBack&) = delete;
  PlatformCallBack& operator=(const PlatformCallBack&) = delete;
  PlatformCallBack(PlatformCallBack&&) = default;
  PlatformCallBack& operator=(PlatformCallBack&&) = default;

  virtual void InvokeWithValue(const lepus::Value& value) {
    if (func_) {
      func_(value);
    }
  };

 private:
  DataCallBackType func_;
  int32_t id_{0};
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_COMMON_PLATFORM_CALL_BACK_H_

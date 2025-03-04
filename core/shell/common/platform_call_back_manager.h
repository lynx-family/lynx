// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_COMMON_PLATFORM_CALL_BACK_MANAGER_H_
#define CORE_SHELL_COMMON_PLATFORM_CALL_BACK_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "core/shell/common/platform_call_back.h"

namespace lynx {
namespace shell {

class PlatformCallBackManager;
class PlatformCallBackHolder {
 public:
  PlatformCallBackHolder(std::unique_ptr<PlatformCallBack> func,
                         int32_t id = -1)
      : platform_call_back_(std::move(func)), id_(id){};

  ~PlatformCallBackHolder() = default;

  int32_t id() const { return id_; }
  bool IsValid() { return id_ != -1; }

  void InvokeWithValue(const lepus::Value& value);

 private:
  std::unique_ptr<PlatformCallBack> platform_call_back_;
  int32_t id_;
};

class PlatformCallBackManager {
 public:
  PlatformCallBackManager() = default;
  std::shared_ptr<PlatformCallBackHolder> CreatePlatformCallBackHolder(
      std::unique_ptr<PlatformCallBack> callback);

  void InvokeWithValue(const std::shared_ptr<PlatformCallBackHolder>& call_back,
                       const lepus::Value& value);
  void EraseCallBack(const std::shared_ptr<PlatformCallBackHolder>& call_back);

  bool HasCallBack(int32_t id);

  void Destroy();

 private:
  std::unordered_map<int, std::shared_ptr<PlatformCallBackHolder>>
      callback_map_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_COMMON_PLATFORM_CALL_BACK_MANAGER_H_

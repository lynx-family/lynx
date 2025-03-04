// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/common/platform_call_back_manager.h"

namespace lynx {
namespace shell {

std::shared_ptr<PlatformCallBackHolder>
PlatformCallBackManager::CreatePlatformCallBackHolder(
    std::unique_ptr<PlatformCallBack> callback) {
  int32_t id = callback->id();
  std::shared_ptr<PlatformCallBackHolder> holder =
      std::make_shared<PlatformCallBackHolder>(std::move(callback), id);
  callback_map_.insert({id, holder});
  return holder;
}

void PlatformCallBackManager::Destroy() { callback_map_.clear(); }

bool PlatformCallBackManager::HasCallBack(int32_t id) {
  return callback_map_.find(id) != callback_map_.end();
}

void PlatformCallBackManager::InvokeWithValue(
    const std::shared_ptr<PlatformCallBackHolder>& call_back,
    const lepus::Value& value) {
  if (HasCallBack(call_back->id())) {
    call_back->InvokeWithValue(value);
  }
}

void PlatformCallBackManager::EraseCallBack(
    const std::shared_ptr<PlatformCallBackHolder>& call_back) {
  if (HasCallBack(call_back->id())) {
    callback_map_.erase(call_back->id());
  }
}

void PlatformCallBackHolder::InvokeWithValue(const lepus::Value& value) {
  if (platform_call_back_) {
    platform_call_back_->InvokeWithValue(value);
  }
}
}  // namespace shell
}  // namespace lynx

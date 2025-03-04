// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shared_data/white_board_runtime_delegate.h"

#include <string>
#include <utility>

#include "core/renderer/events/closure_event_listener.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/shared_data/lynx_white_board.h"

namespace lynx {
namespace tasm {

WhiteBoardRuntimeDelegate::WhiteBoardRuntimeDelegate(
    const std::shared_ptr<WhiteBoard>& white_board)
    : WhiteBoardDelegate(white_board) {}

void WhiteBoardRuntimeDelegate::CallLepusCallbackWithValue(
    const lepus::Value& closure, const lepus::Value& param) {
  DCHECK(false)
      << "WhiteBoardRuntimeDelegate should not receive calls not from js";
}

void WhiteBoardRuntimeDelegate::CallJSApiCallbackWithValue(
    piper::ApiCallBack callback, const lepus::Value& param) {
  if (runtime_actor_) {
    runtime_actor_->Act(
        [callback = std::move(callback), param](auto& runtime) mutable {
          runtime->CallJSApiCallbackWithValue(std::move(callback), param);
        });
  }
}

void WhiteBoardRuntimeDelegate::RemoveJSApiCallback(
    piper::ApiCallBack callback) {
  if (runtime_actor_) {
    runtime_actor_->Act(
        [callback = std::move(callback)](auto& runtime) mutable {
          runtime->EraseJSApiCallback(std::move(callback));
        });
  }
}

void WhiteBoardRuntimeDelegate::CallPlatformCallbackWithValue(
    const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback,
    const lepus::Value& value) {
  callback->InvokeWithValue(value);
}

void WhiteBoardRuntimeDelegate::RemovePlatformCallback(
    const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback) {
  if (runtime_facade_actor_) {
    runtime_facade_actor_->Act(
        [callback](auto& facade) { facade->RemovePlatformCallBack(callback); });
  }
}

}  // namespace tasm
}  // namespace lynx

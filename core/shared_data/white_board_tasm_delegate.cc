// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shared_data/white_board_tasm_delegate.h"

#include <string>
#include <utility>

#include "core/renderer/events/closure_event_listener.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/shared_data/lynx_white_board.h"

namespace lynx {
namespace tasm {

WhiteBoardTasmDelegate::WhiteBoardTasmDelegate(
    TemplateAssembler* tasm, const std::shared_ptr<WhiteBoard>& white_board)
    : WhiteBoardDelegate(white_board), tasm_(tasm) {
  if (tasm_) {
    AddEventListeners(
        tasm->GetContextProxy(runtime::ContextProxy::Type::kJSContext));
  }
}

void WhiteBoardTasmDelegate::CallLepusCallbackWithValue(
    const lepus::Value& closure, const lepus::Value& param) {
  if (tasm_) {
    tasm_->TriggerLepusClosure(closure, param);
  }
}

void WhiteBoardTasmDelegate::CallJSApiCallbackWithValue(
    piper::ApiCallBack callback, const lepus::Value& param) {
  if (tasm_) {
    // Invoke JSApiCallback without remove, js storage callback maybe invoked
    // multiple times.
    tasm_->GetDelegate().CallJSApiCallbackWithValue(std::move(callback), param,
                                                    true);
  }
}

void WhiteBoardTasmDelegate::RemoveJSApiCallback(piper::ApiCallBack callback) {
  if (tasm_) {
    tasm_->GetDelegate().RemoveJSApiCallback(std::move(callback));
  }
}

void WhiteBoardTasmDelegate::CallPlatformCallbackWithValue(
    const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback,
    const lepus::Value& value) {
  callback->InvokeWithValue(value);
}

void WhiteBoardTasmDelegate::RemovePlatformCallback(
    const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback) {
  tasm_->GetDelegate().RemovePlatformCallback(callback);
}
}  // namespace tasm
}  // namespace lynx

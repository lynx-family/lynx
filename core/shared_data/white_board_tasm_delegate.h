// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHARED_DATA_WHITE_BOARD_TASM_DELEGATE_H_
#define CORE_SHARED_DATA_WHITE_BOARD_TASM_DELEGATE_H_

#include <memory>
#include <string>

#include "core/shared_data/white_board_delegate.h"

namespace lynx {
namespace tasm {

class WhiteBoardTasmDelegate : public WhiteBoardDelegate {
 public:
  WhiteBoardTasmDelegate(TemplateAssembler* tasm,
                         const std::shared_ptr<WhiteBoard>& white_board);

  ~WhiteBoardTasmDelegate() override = default;

  void CallLepusCallbackWithValue(const lepus::Value& closure,
                                  const lepus::Value& param) override;

  void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                  const lepus::Value& param) override;
  void RemoveJSApiCallback(piper::ApiCallBack callback) override;

  void CallPlatformCallbackWithValue(
      const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback,
      const lepus::Value& value) override;

  void RemovePlatformCallback(
      const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback)
      override;

  // Move Only.
  WhiteBoardTasmDelegate(const WhiteBoardTasmDelegate&) = delete;
  WhiteBoardTasmDelegate& operator=(const WhiteBoardTasmDelegate&) = delete;
  WhiteBoardTasmDelegate(WhiteBoardTasmDelegate&&) = default;
  WhiteBoardTasmDelegate& operator=(WhiteBoardTasmDelegate&&) = default;

 private:
  TemplateAssembler* tasm_{nullptr};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SHARED_DATA_WHITE_BOARD_TASM_DELEGATE_H_

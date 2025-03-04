// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHARED_DATA_WHITE_BOARD_RUNTIME_DELEGATE_H_
#define CORE_SHARED_DATA_WHITE_BOARD_RUNTIME_DELEGATE_H_

#include <memory>
#include <string>

#include "core/shared_data/white_board_delegate.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace tasm {

class WhiteBoardRuntimeDelegate : public WhiteBoardDelegate {
 public:
  WhiteBoardRuntimeDelegate(const std::shared_ptr<WhiteBoard>& white_board);

  ~WhiteBoardRuntimeDelegate() override = default;

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

  void SetRuntimeActor(
      const std::shared_ptr<shell::LynxActor<runtime::LynxRuntime>>&
          runtime_actor) {
    runtime_actor_ = runtime_actor;
  }

  void SetRuntimeFacadeActor(
      const std::shared_ptr<shell::LynxActor<shell::NativeFacade>>&
          runtime_facade_actor) {
    runtime_facade_actor_ = runtime_facade_actor;
  }

  // Move Only.
  WhiteBoardRuntimeDelegate(const WhiteBoardRuntimeDelegate&) = delete;
  WhiteBoardRuntimeDelegate& operator=(const WhiteBoardRuntimeDelegate&) =
      delete;
  WhiteBoardRuntimeDelegate(WhiteBoardRuntimeDelegate&&) = default;
  WhiteBoardRuntimeDelegate& operator=(WhiteBoardRuntimeDelegate&&) = default;

 private:
  std::shared_ptr<shell::LynxActor<runtime::LynxRuntime>> runtime_actor_{
      nullptr};

  // will always forwards to LynxBackgroundRuntime on platform-level,
  // will always run on JS thread
  std::shared_ptr<shell::LynxActor<shell::NativeFacade>> runtime_facade_actor_{
      nullptr};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SHARED_DATA_WHITE_BOARD_RUNTIME_DELEGATE_H_

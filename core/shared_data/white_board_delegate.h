// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHARED_DATA_WHITE_BOARD_DELEGATE_H_
#define CORE_SHARED_DATA_WHITE_BOARD_DELEGATE_H_

#include <memory>
#include <string>

#include "core/renderer/template_assembler.h"
#include "core/shell/common/platform_call_back_manager.h"

namespace lynx {
namespace tasm {

class WhiteBoard;
class WhiteBoardDelegate
    : public std::enable_shared_from_this<WhiteBoardDelegate> {
 public:
  explicit WhiteBoardDelegate(const std::shared_ptr<WhiteBoard>& white_board);

  virtual ~WhiteBoardDelegate() = default;

  virtual void CallLepusCallbackWithValue(const lepus::Value& closure,
                                          const lepus::Value& param) = 0;

  virtual void CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                          const lepus::Value& param) = 0;

  virtual void RemoveJSApiCallback(piper::ApiCallBack callback) = 0;

  virtual void CallPlatformCallbackWithValue(
      const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback,
      const lepus::Value& value) = 0;

  virtual void RemovePlatformCallback(
      const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback) = 0;

  void SetSessionStorageItem(const std::string& key, const lepus::Value& value);

  lepus::Value GetSessionStorageItem(const std::string& key);

  void SubscribeJSSessionStorage(const std::string& key, double listener_id,
                                 const piper::ApiCallBack& callback);

  void UnsubscribeJSSessionStorage(const std::string& key, double listener_id);

  void SubScribeClientSessionStorage(
      const std::string& key,
      const std::shared_ptr<lynx::shell::PlatformCallBackHolder>& callback);

  void UnsubscribeClientSessionStorage(const std::string& key,
                                       double callback_id);

  // Now, we need both runtime_actor and dispatch_event, this makes
  // white_board_runtime_delegate requiring two steps initialization.
  // After we switch to full event bases impl, we can move this
  // into constructor.
  void AddEventListeners(runtime::ContextProxy* js_context_proxy);

 private:
  std::shared_ptr<WhiteBoard> white_board_{nullptr};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SHARED_DATA_WHITE_BOARD_DELEGATE_H_

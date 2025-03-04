// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JAVA_SCRIPT_DEBUGGER_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JAVA_SCRIPT_DEBUGGER_NG_H_

#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include "base/include/closure.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"

namespace lynx {
namespace devtool {

class JavaScriptDebuggerNG
    : public std::enable_shared_from_this<JavaScriptDebuggerNG> {
 public:
  explicit JavaScriptDebuggerNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
      : devtool_mediator_wp_(devtool_mediator) {}
  virtual ~JavaScriptDebuggerNG() = default;

  void SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
    devtool_platform_facade_wp_ = devtool_platform_facade;
  }

  virtual int GetViewId() { return kDefaultViewID; }

  virtual void DispatchMessage(const std::string& message,
                               const std::string& session_id = "") = 0;
  void SendResponse(const std::string& message) {
    // TODO(lqy): change to message sender
    std::lock_guard<std::mutex> lock(mutex_);
    if (attached_) {
      SendMessage(message);
    } else {
      // CDP messages are sent from the JS/TASM thread and devtool is attached
      // on the main thread, so that messages may be sent before devtool is
      // attached.
      message_buf_.push(message);
    }
  }

  void SendMessage(const std::string& message) {
    auto sp = devtool_mediator_wp_.lock();
    CHECK_NULL_AND_LOG_RETURN(sp, "js debug: devtool_mediator_ is null");
    Json::Value mes;
    Json::Reader reader;
    reader.parse(message, mes, false);
    sp->SendCDPEvent(mes);
  }

  void OnAttached() {
    std::lock_guard<std::mutex> lock(mutex_);
    attached_ = true;
    FlushMessageBuf();
  }

  void FlushMessageBuf() {
    while (!message_buf_.empty()) {
      SendMessage(message_buf_.front());
      message_buf_.pop();
    }
  }

  virtual void RunOnTargetThread(base::closure&& closure,
                                 bool run_now = true) = 0;

 protected:
  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  std::weak_ptr<DevToolPlatformFacade> devtool_platform_facade_wp_;

  bool attached_{false};
  std::mutex mutex_;
  std::queue<std::string> message_buf_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JAVA_SCRIPT_DEBUGGER_NG_H_

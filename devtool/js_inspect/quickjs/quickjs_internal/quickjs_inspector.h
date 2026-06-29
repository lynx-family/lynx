// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTOR_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTOR_H_

#include <memory>
#include <string>

#include "quickjs/include/quickjs.h"

namespace quickjs_inspector {
class QJSInspectorSession {
 public:
  virtual ~QJSInspectorSession() = default;
  virtual void DispatchProtocolMessage(const std::string& message) = 0;
  virtual void SchedulePauseOnNextStatement(const std::string& reason) = 0;
  virtual void CancelPauseOnNextStatement() = 0;
  virtual void SetEnableConsoleInspect(bool enable) = 0;
};

class QJSInspectorClient {
 public:
  virtual ~QJSInspectorClient() = default;

  virtual void RunMessageLoopOnPause(const std::string& group_id) {}
  virtual void QuitMessageLoopOnPause() {}

  // Whether need to use the full functionality.
  // If return false, the Quickjs can send scriptParsed and consoleAPICalled
  // messages after enabled, but cannot pause on breakpoints.
  virtual bool IsFullFuncEnabled() { return true; }
};

class QJSInspector {
 public:
  static std::unique_ptr<QJSInspector> Create(LEPUSContext* ctx,
                                              QJSInspectorClient* client,
                                              const std::string& group_id,
                                              const std::string& name);
  virtual ~QJSInspector() = default;
  class QJSChannel {
   public:
    virtual ~QJSChannel() = default;
    virtual void SendResponse(int call_id, const std::string& message) = 0;
    virtual void SendNotification(const std::string& message) = 0;
    virtual void OnConsoleMessage(const std::string& message,
                                  const std::string& url) = 0;
  };
  virtual std::unique_ptr<QJSInspectorSession> Connect(
      QJSChannel* channel, const std::string& group_id, int32_t session_id) = 0;
};
}  // namespace quickjs_inspector

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTOR_H_

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_INSPECTOR_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_INSPECTOR_H_
#include <memory>
#include <queue>
#include <string>

#include "base/include/base_export.h"

namespace lynx {
namespace lepus {
class Context;
}

}  // namespace lynx
namespace lepus_inspector {
class LepusInspectorSession {
 public:
  virtual ~LepusInspectorSession() = default;
  virtual void dispatchProtocolMessage(const std::string& message) = 0;
  virtual void schedulePauseOnNextStatement(
      const std::string& breakReason, const std::string& breakDetails) = 0;
  virtual void cancelPauseOnNextStatement() = 0;
  virtual void setEnableConsoleInspect(bool) = 0;
  virtual bool getEnableConsoleInspect() const = 0;
};

class LepusInspectorClient {
 public:
  virtual ~LepusInspectorClient() = default;
  virtual void runMessageLoopOnPause(const std::string& group_id) {}
  virtual void quitMessageLoopOnPause() {}
  virtual std::queue<std::string> getMessageFromFrontend() = 0;
};

class LepusInspector {
 public:
  static BASE_EXPORT_FOR_DEVTOOL std::unique_ptr<LepusInspector> create(
      lynx::lepus::Context* ctx, LepusInspectorClient*);
  virtual ~LepusInspector() = default;
  class LepusChannel {
   public:
    virtual ~LepusChannel() = default;
    virtual void sendResponse(int callId, const std::string& message) = 0;
    virtual void sendNotification(const std::string& message) = 0;
    virtual void flushProtocolNotifications() = 0;
  };
  virtual std::unique_ptr<LepusInspectorSession> connect(
      int contextGroupId, LepusChannel*, const std::string& state) = 0;
  virtual void SetInspectorClient(LepusInspectorClient* client,
                                  const std::string&) = 0;
};

}  // namespace lepus_inspector
#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_INSPECTOR_H_

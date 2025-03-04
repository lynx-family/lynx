// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTOR_NG_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTOR_NG_H_

#include <memory>
#include <queue>
#include <string>

namespace lynx {
namespace lepus {
class Context;
}  // namespace lepus
}  // namespace lynx

namespace lepus_inspector {

class LepusInspectorSessionNG {
 public:
  virtual ~LepusInspectorSessionNG() = default;
  virtual void DispatchProtocolMessage(const std::string& message) = 0;
  virtual void SchedulePauseOnNextStatement(const std::string& reason) = 0;
  virtual void CancelPauseOnNextStatement() = 0;
};

class LepusInspectorClientNG {
 public:
  virtual ~LepusInspectorClientNG() = default;
  virtual void RunMessageLoopOnPause() {}
  virtual void QuitMessageLoopOnPause() {}
};

class LepusInspectorNG {
 public:
  static std::unique_ptr<LepusInspectorNG> Create(
      lynx::lepus::Context* context, LepusInspectorClientNG* client,
      const std::string& name);
  virtual ~LepusInspectorNG() = default;
  class LepusChannel {
   public:
    virtual ~LepusChannel() = default;
    virtual void SendResponse(int call_id, const std::string& message) = 0;
    virtual void SendNotification(const std::string& message) = 0;
  };
  virtual std::unique_ptr<LepusInspectorSessionNG> Connect(
      LepusChannel* channel) = 0;
  virtual void SetDebugInfo(const std::string& url,
                            const std::string& debug_info) = 0;
};

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTOR_NG_H_

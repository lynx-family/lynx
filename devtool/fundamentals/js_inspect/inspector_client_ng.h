// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_NG_H_
#define DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_NG_H_

#include <functional>
#include <memory>
#include <string>

namespace lynx {
namespace devtool {
class InspectorClientDelegate;

// Abstract the different JS engines.
class InspectorClientNG
    : public std::enable_shared_from_this<InspectorClientNG> {
 public:
  InspectorClientNG() = default;
  virtual ~InspectorClientNG() = default;

  void SetInspectorClientDelegate(
      const std::shared_ptr<InspectorClientDelegate>& delegate) {
    delegate_wp_ = delegate;
  }

  // Set whether need to stop the executing of js at entry.
  virtual void SetStopAtEntry(bool stop_at_entry, int instance_id) = 0;

  // The following two functions are used to pass CDP messages in the JS engine
  // and the DevTool. They are named in the same way as the interfaces declared
  // in V8 (see v8_inspector.h).
  // SendResponse: JS engine -> DevTool
  // DispatchMessage: DevTool -> JS engine
  void SendResponse(const std::string& message, int instance_id);
  virtual void DispatchMessage(const std::string& message, int instance_id) = 0;

  // Only worked on Quickjs.
  virtual void SetEnableConsoleInspect(bool enable, int instance_id) {}
  virtual void GetConsoleObject(
      const std::string& object_id, const std::string& group_id,
      std::function<void(const std::string&)> callback) {}
  // Only worked on Quickjs ends.

 protected:
  std::weak_ptr<InspectorClientDelegate> delegate_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_NG_H_

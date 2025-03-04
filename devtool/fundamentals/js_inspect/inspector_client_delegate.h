// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_H_
#define DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_H_

#include <memory>
#include <string>

#include "devtool/fundamentals/js_inspect/inspector_client_ng.h"
#include "devtool/fundamentals/js_inspect/inspector_client_quickjs_delegate.h"
#include "devtool/fundamentals/js_inspect/inspector_client_v8_delegate.h"

namespace lynx {
namespace devtool {

// Communicate with InspectorClientNG directly to connect the JS engine and
// the DevTool. All functions declared here are common to the different JS
// engines and must be called on the JS thread.
class InspectorClientDelegate : public InspectorClientV8Delegate,
                                public InspectorClientQJSDelegate {
 public:
  virtual ~InspectorClientDelegate() = default;

  void SetInspectorClient(const std::shared_ptr<InspectorClientNG>& client) {
    client_wp_ = client;
  }

  // Set whether need to stop the executing of js at entry.
  void SetStopAtEntry(bool stop_at_entry, int instance_id) {
    auto sp = client_wp_.lock();
    if (sp != nullptr) {
      sp->SetStopAtEntry(stop_at_entry, instance_id);
    }
  }
  // Called when a message session is destroyed.
  virtual void OnSessionDestroyed(int instance_id,
                                  const std::string& group_id) {}
  // Called when a js context is destroyed.
  virtual void OnContextDestroyed(const std::string& group_id, int context_id) {
  }

  // The following two functions are used to pass CDP messages in the JS
  // engine and the DevTool. They are named in the same way as the interfaces
  // declared in V8 (see v8_inspector.h). SendResponse: JS engine -> DevTool
  // DispatchMessage: DevTool -> JS engine
  virtual void SendResponse(const std::string& message, int instance_id) = 0;
  void DispatchMessage(const std::string& message, int instance_id) {
    auto sp = client_wp_.lock();
    if (sp != nullptr) {
      sp->DispatchMessage(message, instance_id);
    }
  }

  // Called when a breakpoint is triggered or exited.
  virtual void RunMessageLoopOnPause(const std::string& group_id) = 0;
  virtual void QuitMessageLoopOnPause() = 0;

 protected:
  std::weak_ptr<InspectorClientNG> client_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_H_

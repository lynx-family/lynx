// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INSPECTOR_CLIENT_IMPL_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INSPECTOR_CLIENT_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/fundamentals/js_inspect/inspector_client_ng.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector.h"

namespace lynx {
namespace devtool {

class QJSInspectorClientImpl;

class QJSChannelImplNG : public quickjs_inspector::QJSInspector::QJSChannel {
 public:
  QJSChannelImplNG(
      const std::unique_ptr<quickjs_inspector::QJSInspector>& inspector,
      const std::shared_ptr<QJSInspectorClientImpl>& client,
      const std::string& group_id, int instance_id);
  ~QJSChannelImplNG() override = default;

  const std::string& GroupId() { return group_id_; }

  void SendResponse(int call_id, const std::string& message) override;
  void SendNotification(const std::string& message) override;
  void OnConsoleMessage(const std::string& message,
                        int32_t runtime_id) override;

  void DispatchProtocolMessage(const std::string& message);
  void SchedulePauseOnNextStatement(const std::string& reason);
  void CancelPauseOnNextStatement();
  void SetEnableConsoleInspect(bool enable);

 private:
  void SendResponseToClient(const std::string& message);

  std::unique_ptr<quickjs_inspector::QJSInspectorSession> session_;
  std::weak_ptr<QJSInspectorClientImpl> client_wp_;

  int instance_id_;
  std::string group_id_;
};

class QJSInspectorClientImpl : public quickjs_inspector::QJSInspectorClient,
                               public InspectorClientNG {
 public:
  QJSInspectorClientImpl() = default;
  ~QJSInspectorClientImpl() override = default;

  void RunMessageLoopOnPause(const std::string& group_id) override;
  void QuitMessageLoopOnPause() override;
  bool IsFullFuncEnabled() override;

  void SetStopAtEntry(bool stop_at_entry, int instance_id) override;
  void DispatchMessage(const std::string& message, int instance_id) override;

  void SetEnableConsoleInspect(bool enable, int instance_id) override;
  void GetConsoleObject(
      const std::string& object_id, const std::string& group_id,
      std::function<void(const std::string&)> callback) override;
  void OnConsoleMessage(const std::string& message, int instance_id,
                        int runtime_id);

  std::string InitInspector(LEPUSContext* context, const std::string& group_id,
                            const std::string& name = "");
  void ConnectSession(int instance_id, const std::string& group_id);
  void DisconnectSession(int instance_id);
  // Only be called when preparing to destroy LEPUSContext. The param is the
  // group_id after mapping.
  void DestroyInspector(const std::string& group_id);

  // Set a callback to determine whether need to use the full functionality.
  // If the callback return false, the Quickjs can send scriptParsed and
  // consoleAPICalled messages after enabled, but cannot pause on breakpoints.
  void SetFullFuncEnableCallback(std::function<bool()> callback);

  // The following two functions only be called when the LEPUSContext won't be
  // destroyed but need to remove some scripts or console messages saved in
  // inspector. There's no need to call them if the LEPUSContext will be
  // destroyed.
  // params:
  // group_id: The group_id after mapping.
  // url: Url of the script needs to be removed.
  // runtime_id: An argument of console messages when using "lynxConsole".
  void RemoveScript(const std::string& group_id, const std::string& url);
  void RemoveConsole(const std::string& group_id, int runtime_id);

 private:
  void SetContext(LEPUSContext* context, const std::string& group_id);
  void CreateQJSInspector(LEPUSContext* context, const std::string& group_id,
                          const std::string& name);

  std::string MapGroupId(const std::string& group_id);

  std::unordered_map<int, std::shared_ptr<QJSChannelImplNG>>
      channels_;  // instance_id -> QJSChannelImplNG
  std::unordered_map<std::string, LEPUSContext*>
      contexts_;  // group_id -> LEPUSContext
  std::unordered_map<std::string,
                     std::unique_ptr<quickjs_inspector::QJSInspector>>
      inspectors_;  // group_id -> QJSInspector

  std::function<bool()> full_func_enable_callback_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INSPECTOR_CLIENT_IMPL_H_

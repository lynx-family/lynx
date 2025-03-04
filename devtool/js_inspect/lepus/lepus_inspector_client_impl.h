// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INSPECTOR_CLIENT_IMPL_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INSPECTOR_CLIENT_IMPL_H_

#include <memory>
#include <string>

#include "devtool/fundamentals/js_inspect/inspector_client_ng.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspector_ng.h"

namespace lynx {
namespace devtool {

class LepusInspectorClientImpl;

class LepusChannelImplNG
    : public lepus_inspector::LepusInspectorNG::LepusChannel {
 public:
  LepusChannelImplNG(
      const std::unique_ptr<lepus_inspector::LepusInspectorNG>& inspector,
      const std::shared_ptr<LepusInspectorClientImpl>& client);
  ~LepusChannelImplNG() override = default;

  void SendResponse(int call_id, const std::string& message) override;
  void SendNotification(const std::string& message) override;

  void DispatchProtocolMessage(const std::string& message);
  void SchedulePauseOnNextStatement(const std::string& reason);
  void CancelPauseOnNextStatement();

 private:
  void SendResponseToClient(const std::string& message);

  std::unique_ptr<lepus_inspector::LepusInspectorSessionNG> session_;
  std::weak_ptr<LepusInspectorClientImpl> client_wp_;
};

class LepusInspectorClientImpl : public lepus_inspector::LepusInspectorClientNG,
                                 public InspectorClientNG {
 public:
  LepusInspectorClientImpl() = default;
  ~LepusInspectorClientImpl() override = default;

  void RunMessageLoopOnPause() override;
  void QuitMessageLoopOnPause() override;

  void SetStopAtEntry(bool stop_at_entry, int instance_id) override;
  void DispatchMessage(const std::string& message, int instance_id) override;

  void InitInspector(lepus::Context* context, const std::string& name = "");
  void SetDebugInfo(const std::string& url, const std::string& debug_info);
  void ConnectSession();
  void DisconnectSession();
  void DestroyInspector();

 private:
  std::shared_ptr<LepusChannelImplNG> channel_;
  std::unique_ptr<lepus_inspector::LepusInspectorNG> inspector_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INSPECTOR_CLIENT_IMPL_H_

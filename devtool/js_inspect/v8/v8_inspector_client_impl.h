// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_V8_V8_INSPECTOR_CLIENT_IMPL_H_
#define DEVTOOL_JS_INSPECT_V8_V8_INSPECTOR_CLIENT_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/fundamentals/js_inspect/inspector_client_ng.h"
#include "v8-inspector.h"

namespace lynx {
namespace devtool {

class V8InspectorClientImpl;

class V8ChannelImpl : public v8_inspector::V8Inspector::Channel {
 public:
  V8ChannelImpl(const std::unique_ptr<v8_inspector::V8Inspector>& inspector,
                const std::shared_ptr<V8InspectorClientImpl>& client,
                int instance_id, int group_id);
  ~V8ChannelImpl() = default;

  int GroupId() { return group_id_; }

  void sendResponse(
      int callId, std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void flushProtocolNotifications() override {}

  void DispatchProtocolMessage(const std::string& message);
  void SchedulePauseOnNextStatement(const std::string& reason);
  void CancelPauseOnNextStatement();

 private:
  void SendResponseToClient(
      const std::unique_ptr<v8_inspector::StringBuffer>& message);

  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
  std::weak_ptr<V8InspectorClientImpl> client_wp_;

  int instance_id_;
  int group_id_;
};

class V8InspectorClientImpl : public v8_inspector::V8InspectorClient,
                              public InspectorClientNG {
 public:
  V8InspectorClientImpl() = default;
  ~V8InspectorClientImpl() = default;

  void runMessageLoopOnPause(int contextGroupId) override;
  void quitMessageLoopOnPause() override;
  v8::Local<v8::Context> ensureDefaultContextInGroup(
      int contextGroupId) override;

  // Add timestamp to console messages.
  double currentTimeMS() override;
  // Memory panel: Allocation instrumentation on timeline.
  void startRepeatingTimer(double interval, TimerCallback callback,
                           void* data) override;
  void cancelTimer(void* data) override;

  void SetStopAtEntry(bool stop_at_entry, int instance_id) override;
  void DispatchMessage(const std::string& message, int instance_id) override;

  // Return the group_id after mapping.
  int InitInspector(v8::Isolate* isolate, v8::Local<v8::Context> ctx,
                    const std::string& group_id, const std::string& name = "");
  // The param group_id is the value after mapping.
  void ConnectSession(int instance_id, int group_id);
  void DisconnectSession(int instance_id);
  // Only be called when preparing to destroy v8::Context. The param is the
  // group_id after mapping.
  void DestroyContext(int group_id);
  // Only be called when preparing to destroy v8::Context. The param is the
  // group_id before mapping(cannot be "-1").
  void DestroyContext(const std::string& group_id);
  // Only be called when preparing to destroy v8::Isolate.
  void DestroyInspector();

 private:
  void CreateV8Inspector();
  void ContextCreated(v8::Local<v8::Context> context, int group_id,
                      const std::string& name);
  void ContextDestroyed(int group_id);
  void DestroyAllSessions();

  int MapGroupStrToNum(const std::string& group_string);
  void RemoveGroupMapping(int group_num);

  v8::Isolate* isolate_{nullptr};
  std::unique_ptr<v8_inspector::V8Inspector> inspector_;

  std::unordered_map<int, std::shared_ptr<V8ChannelImpl>> channels_;
  std::unordered_map<
      int,
      v8::Persistent<v8::Context, v8::CopyablePersistentTraits<v8::Context>>>
      contexts_;

  std::unordered_map<std::string, int> group_string_to_number_;
  std::unordered_map<int, std::string> group_number_to_string_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_V8_V8_INSPECTOR_CLIENT_IMPL_H_

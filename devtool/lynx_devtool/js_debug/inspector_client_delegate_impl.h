// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_INSPECTOR_CLIENT_DELEGATE_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_INSPECTOR_CLIENT_DELEGATE_IMPL_H_

#include <set>
#include <string>

#include "devtool/base_devtool/native/js_inspect/inspector_client_delegate_base_impl.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"

namespace lynx {
namespace devtool {
class InspectorClientDelegateImpl;
class JavaScriptDebuggerNG;

class InspectorClientDelegateProvider {
 public:
  // Must be called on JS thread.
  static InspectorClientDelegateProvider* GetInstance();
  std::shared_ptr<InspectorClientDelegateImpl> GetDelegate(
      const std::string& vm_type);

  InspectorClientDelegateProvider(const InspectorClientDelegateProvider&) =
      delete;
  InspectorClientDelegateProvider& operator=(
      const InspectorClientDelegateProvider&) = delete;
  InspectorClientDelegateProvider(InspectorClientDelegateProvider&&) = delete;
  InspectorClientDelegateProvider& operator=(
      InspectorClientDelegateProvider&&) = delete;

 private:
  InspectorClientDelegateProvider() = default;

  std::unordered_map<std::string, std::shared_ptr<InspectorClientDelegateImpl>>
      delegates_;
};

struct JSDebugBundle {
  JSDebugBundle(int view_id, bool single_group,
                const std::shared_ptr<JavaScriptDebuggerNG>& debugger)
      : view_id_(view_id), single_group_(single_group), debugger_(debugger) {
    script_manager_ = std::make_unique<ScriptManagerNG>();
  }
  JSDebugBundle() {}

  int view_id_{kErrorViewID};
  int64_t runtime_id_{kErrorViewID};
  std::string group_id_{kErrorGroupStr};
  bool single_group_{true};
  bool enable_console_inspect_{false};
  std::weak_ptr<JavaScriptDebuggerNG> debugger_;
  std::unique_ptr<ScriptManagerNG> script_manager_;
};

class InspectorClientDelegateImpl : public InspectorClientDelegateBaseImpl {
 public:
  InspectorClientDelegateImpl(const std::string& vm_type);
  ~InspectorClientDelegateImpl() override = default;

  void OnContextDestroyed(const std::string& group_id, int context_id) override;
  void SendResponse(const std::string& message, int instance_id) override;

  void OnConsoleMessage(const std::string& message, int instance_id,
                        int runtime_id) override;

  void InsertDebugger(const std::shared_ptr<JavaScriptDebuggerNG>& debugger,
                      bool single_group);
  void RemoveDebugger(int view_id);

  void SetTargetId(const std::string& target_id) { target_id_ = target_id; }

  void OnInspectorInited(int view_id, int64_t runtime_id,
                         const std::string& group_id);
  void OnRuntimeDestroyed(int view_id, int64_t runtime_id);
  void OnTargetCreated();
  void OnTargetDestroyed();

  void DispatchInitMessage(int view_id, bool runtime_enable);

  void FlushConsoleMessages(int view_id);
  void GetConsoleObject(const std::string& object_id, int view_id,
                        bool need_stringify, int callback_id);

 private:
  void PostTask(int instance_id, std::function<void()>&& closure) override;

  void InsertRuntimeId(int64_t runtime_id, int view_id);
  void RemoveRuntimeId(int64_t runtime_id);
  int GetViewIdByRuntimeId(int64_t runtime_id);

  void InsertViewIdToGroup(const std::string& group_id, int view_id);
  void RemoveViewIdFromGroup(const std::string& group_id, int view_id);
  void RemoveGroup(const std::string& group_id);
  const std::set<int>& GetViewIdInGroup(const std::string& group_id);

  void InsertScriptId(int view_id, int script_id);
  void InsertInvalidScriptId(int view_id);
  bool IsScriptIdInvalid(int script_id);

  bool IsViewInSingleGroup(int view_id);

  const std::weak_ptr<JavaScriptDebuggerNG>& GetDebuggerByViewId(int view_id);
  const std::unique_ptr<ScriptManagerNG>& GetScriptManagerByViewId(int view_id);

  void SetEnableConsoleInspect(int view_id);
  void SetEnableConsoleInspect(bool enable, int view_id);

  std::string PrepareDispatchMessage(rapidjson::Document& message,
                                     int instance_id) override;
  std::string PrepareResponseMessage(const std::string& message,
                                     int instance_id) override;
  bool HandleMessageConsoleAPICalled(rapidjson::Document& message);
  bool HandleMessageConsoleAPICalledFromV8(rapidjson::Document& message);
  void HandleMessageConsoleAPICalledFromQuickjs(rapidjson::Document& message);
  void HandleMessageConsoleAPICalledFromLepus(rapidjson::Document& message);

  std::string GenMessageCallFunctionOn(const std::string& object_id,
                                       int message_id = 0);
  void SendMessageContextDestroyed(int view_id, int context_id);
  void SendMessageContextCleared(int view_id);
  void SendMessageRemoveScripts(int view_id);

  int GetScriptViewId(const std::string& script_url);

  std::unordered_map<int, JSDebugBundle> view_id_to_bundle_;
  std::unordered_map<int64_t, int> runtime_id_to_view_id_;
  std::unordered_map<std::string, std::set<int>> group_id_to_view_id_;
  std::set<int> invalid_script_ids_;

  std::string target_id_;
  bool target_created_{false};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_INSPECTOR_CLIENT_DELEGATE_IMPL_H_

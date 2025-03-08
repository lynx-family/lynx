// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/inspector_client_delegate_impl.h"

#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"

namespace lynx {
namespace devtool {

InspectorClientDelegateProvider*
InspectorClientDelegateProvider::GetInstance() {
  static thread_local InspectorClientDelegateProvider instance_;
  return &instance_;
}

std::shared_ptr<InspectorClientDelegateImpl>
InspectorClientDelegateProvider::GetDelegate(const std::string& vm_type) {
  if (vm_type == kKeyEngineLepus) {
    return std::make_shared<InspectorClientDelegateImpl>(vm_type);
  }
  if (delegates_.find(vm_type) == delegates_.end()) {
    delegates_[vm_type] =
        std::make_shared<InspectorClientDelegateImpl>(vm_type);
  }
  return delegates_[vm_type];
}

InspectorClientDelegateImpl::InspectorClientDelegateImpl(
    const std::string& vm_type)
    : InspectorClientDelegateBaseImpl(vm_type) {}

void InspectorClientDelegateImpl::OnContextDestroyed(
    const std::string& group_id, int context_id) {
  auto& view_set = GetViewIdInGroup(group_id);
  if (!view_set.empty()) {
    // When this function is called, we can confirm that there is only one view
    // in the group (reload/destroy), or all views in this group is going to be
    // destroyed, so we can send only one CDP message to the frontend.
    SendMessageContextDestroyed(*(view_set.begin()), context_id);
  }
  // When using single group, the group_id will be changed after reloading, so
  // we need to remove the items saved in group_id_to_view_id_ with the old
  // group_id.
  RemoveGroup(group_id);
}

void InspectorClientDelegateImpl::SendResponse(const std::string& message,
                                               int instance_id) {
  auto debugger = GetDebuggerByViewId(instance_id).lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
  std::string res = PrepareResponseMessage(message, instance_id);
  if (!res.empty()) {
    debugger->SendResponse(res);
  }
}

void InspectorClientDelegateImpl::OnConsoleMessage(const std::string& message,
                                                   int instance_id,
                                                   int runtime_id) {
  int mes_view_id = GetViewIdByRuntimeId(runtime_id);
  if (runtime_id == kDefaultGlobalRuntimeID || mes_view_id == instance_id) {
    auto debugger = GetDebuggerByViewId(instance_id).lock();
    CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
    auto js_debugger =
        std::static_pointer_cast<InspectorJavaScriptDebuggerImpl>(debugger);
    js_debugger->OnConsoleMessage(message);
  }
}

void InspectorClientDelegateImpl::InsertDebugger(
    const std::shared_ptr<JavaScriptDebuggerNG>& debugger, bool single_group) {
  int view_id = debugger->GetViewId();
  auto it = view_id_to_bundle_.find(view_id);
  if (it == view_id_to_bundle_.end()) {
    LOGI("js debug: InspectorClientDelegateImpl::InsertDebugger, this: "
         << this << ", debugger: " << debugger << ", view_id: " << view_id);
    view_id_to_bundle_[view_id] =
        JSDebugBundle(view_id, single_group, debugger);
  }
}

void InspectorClientDelegateImpl::RemoveDebugger(int view_id) {
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    // OnRuntimeDestroyed() may cannot be triggered if debugger is destructed
    // earlier, so we need to do these here too.
    runtime_id_to_view_id_.erase(it->second.runtime_id_);
    InsertInvalidScriptId(view_id);

    SendMessageRemoveScripts(view_id);
    RemoveViewIdFromGroup(it->second.group_id_, view_id);
    view_id_to_bundle_.erase(view_id);
  }
}

void InspectorClientDelegateImpl::OnInspectorInited(
    int view_id, int64_t runtime_id, const std::string& group_id) {
  InsertViewIdToGroup(group_id, view_id);
  InsertRuntimeId(runtime_id, view_id);
  SetEnableConsoleInspect(view_id);
}

void InspectorClientDelegateImpl::OnRuntimeDestroyed(int view_id,
                                                     int64_t runtime_id) {
  RemoveRuntimeId(runtime_id);
  if (!IsViewInSingleGroup(view_id)) {
    // When using shared-context, we must clear the messages displayed in the
    // Console panel first, otherwise the messages generated in the progress of
    // destroying will be sent twice: the first time is when they are first
    // generated, and the second time is all messages saved in the same context
    // will be sent after reloading.
    // TODO(lqy): tricky...
    SendMessageContextCleared(view_id);
  }
  // TODO(lqy): If using reloadTemplate, we also need to call this function
  // when reloading.
  InsertInvalidScriptId(view_id);
}

void InspectorClientDelegateImpl::OnTargetCreated() {
  if (target_created_) {
    return;
  }
  auto sp = GetDebuggerByViewId(kDefaultViewID).lock();
  if (sp != nullptr) {
    sp->SendResponse(GenMessageTargetCreated(target_id_, target_id_));
    sp->SendResponse(
        GenMessageAttachedToTarget(target_id_, target_id_, target_id_));
  }
  target_created_ = true;
}

void InspectorClientDelegateImpl::OnTargetDestroyed() {
  if (!target_created_) {
    return;
  }
  auto it = view_id_to_bundle_.find(kDefaultViewID);
  if (it != view_id_to_bundle_.end()) {
    auto sp = it->second.debugger_.lock();
    if (sp != nullptr) {
      // Send messages directly without executing PrepareResponseMessage.
      sp->SendResponse(GenMessageDetachedFromTarget(target_id_));
      sp->SendResponse(GenMessageTargetDestroyed(target_id_));
    }
  }
  target_created_ = false;
}

void InspectorClientDelegateImpl::DispatchInitMessage(int view_id,
                                                      bool runtime_enable) {
  InspectorClientDelegateBaseImpl::DispatchInitMessage(
      view_id, GetScriptManagerByViewId(view_id), runtime_enable);
}

void InspectorClientDelegateImpl::FlushConsoleMessages(int view_id) {
  if (vm_type_ != kKeyEngineQuickjs) {
    return;
  }
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    it->second.enable_console_inspect_ = true;
    SetEnableConsoleInspect(true, view_id);
  }
}

void InspectorClientDelegateImpl::GetConsoleObject(const std::string& object_id,
                                                   int view_id,
                                                   bool need_stringify,
                                                   int callback_id) {
  if (need_stringify) {
    DispatchMessage(GenMessageCallFunctionOn(object_id, callback_id), view_id);
  } else {
    auto sp = client_wp_.lock();
    CHECK_NULL_AND_LOG_RETURN(sp, "js debug: client is null");
    auto it = view_id_to_bundle_.find(view_id);
    if (it != view_id_to_bundle_.end()) {
      auto debugger = it->second.debugger_.lock();
      CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
      auto js_debugger =
          std::static_pointer_cast<InspectorJavaScriptDebuggerImpl>(debugger);
      sp->GetConsoleObject(object_id, it->second.group_id_,
                           [js_debugger = std::move(js_debugger),
                            callback_id](const std::string& detail) {
                             js_debugger->OnConsoleObject(detail, callback_id);
                           });
    }
  }
}

void InspectorClientDelegateImpl::PostTask(int instance_id,
                                           std::function<void()>&& closure) {
  // LynxDevToolMediator and InspectorJavaScriptDebuggerImpl will be destroyed
  // after LynxView is destroyed, while InspectorClientDelegateImpl is a thread
  // local instance. Therefore, we should use InspectorJavaScriptDebuggerImpl
  // instance corresponding to the instance_id to post task instead of storing a
  // fixed weak_ptr of LynxDevToolMediator.
  auto sp = GetDebuggerByViewId(instance_id).lock();
  CHECK_NULL_AND_LOG_RETURN(sp, "js debug: debugger is null");
  sp->RunOnTargetThread(std::move(closure));
}

void InspectorClientDelegateImpl::InsertRuntimeId(int64_t runtime_id,
                                                  int view_id) {
  // Runtime may be destroyed after reloading, so we need to update
  // runtime_id_to_view_id_ and view_id_to_bundle_.
  runtime_id_to_view_id_[runtime_id] = view_id;
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    it->second.runtime_id_ = runtime_id;
  }
}

void InspectorClientDelegateImpl::RemoveRuntimeId(int64_t runtime_id) {
  runtime_id_to_view_id_.erase(runtime_id);
}

int InspectorClientDelegateImpl::GetViewIdByRuntimeId(int64_t runtime_id) {
  auto it = runtime_id_to_view_id_.find(runtime_id);
  if (it != runtime_id_to_view_id_.end()) {
    return it->second;
  }
  return kErrorViewID;
}

void InspectorClientDelegateImpl::InsertViewIdToGroup(
    const std::string& group_id, int view_id) {
  // The group_id will be changed after reloading when using single group, so we
  // need to update group_id_to_view_id_ and view_id_to_bundle_.
  auto it = group_id_to_view_id_.find(group_id);
  if (it == group_id_to_view_id_.end()) {
    group_id_to_view_id_[group_id] = std::set<int>({view_id});
  } else {
    group_id_to_view_id_[group_id].emplace(view_id);
  }
  auto bundle_it = view_id_to_bundle_.find(view_id);
  if (bundle_it != view_id_to_bundle_.end()) {
    bundle_it->second.group_id_ = group_id;
  }
}

void InspectorClientDelegateImpl::RemoveViewIdFromGroup(
    const std::string& group_id, int view_id) {
  auto it = group_id_to_view_id_.find(group_id);
  if (it == group_id_to_view_id_.end()) {
    return;
  }
  it->second.erase(view_id);
  if (it->second.empty()) {
    group_id_to_view_id_.erase(it);
  }
}

void InspectorClientDelegateImpl::RemoveGroup(const std::string& group_id) {
  group_id_to_view_id_.erase(group_id);
}

const std::set<int>& InspectorClientDelegateImpl::GetViewIdInGroup(
    const std::string& group_id) {
  auto it = group_id_to_view_id_.find(group_id);
  if (it != group_id_to_view_id_.end()) {
    return it->second;
  }
  return group_id_to_view_id_[kErrorGroupStr];
}

void InspectorClientDelegateImpl::InsertScriptId(int view_id, int script_id) {
  if (vm_type_ != kKeyEngineV8 || view_id == kErrorViewID ||
      IsScriptIdInvalid(script_id)) {
    return;
  }
  auto it = view_id_to_bundle_.find(view_id);
  if (it == view_id_to_bundle_.end()) {
    invalid_script_ids_.emplace(script_id);
    return;
  }
  if (!it->second.single_group_ && it->second.script_manager_ != nullptr) {
    it->second.script_manager_->InsertScriptId(script_id);
  }
}

void InspectorClientDelegateImpl::InsertInvalidScriptId(int view_id) {
  if (vm_type_ != kKeyEngineV8) {
    return;
  }
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end() && !it->second.single_group_ &&
      it->second.script_manager_ != nullptr) {
    const std::set<int>& script_ids =
        it->second.script_manager_->GetScriptIds();
    for (const auto& id : script_ids) {
      invalid_script_ids_.emplace(id);
    }
    it->second.script_manager_->ClearScriptIds();
  }
}

bool InspectorClientDelegateImpl::IsScriptIdInvalid(int script_id) {
  if (vm_type_ != kKeyEngineV8) {
    return false;
  }
  return invalid_script_ids_.find(script_id) != invalid_script_ids_.end();
}

bool InspectorClientDelegateImpl::IsViewInSingleGroup(int view_id) {
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    return it->second.single_group_;
  }
  return true;
}

const std::weak_ptr<JavaScriptDebuggerNG>&
InspectorClientDelegateImpl::GetDebuggerByViewId(int view_id) {
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    return it->second.debugger_;
  }
  return view_id_to_bundle_[kErrorViewID].debugger_;
}

const std::unique_ptr<ScriptManagerNG>&
InspectorClientDelegateImpl::GetScriptManagerByViewId(int view_id) {
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    return it->second.script_manager_;
  }
  return view_id_to_bundle_[kErrorViewID].script_manager_;
}

void InspectorClientDelegateImpl::SetEnableConsoleInspect(int view_id) {
  if (vm_type_ == kKeyEngineQuickjs) {
    auto it = view_id_to_bundle_.find(view_id);
    if (it != view_id_to_bundle_.end()) {
      SetEnableConsoleInspect(it->second.enable_console_inspect_, view_id);
    }
  }
}

void InspectorClientDelegateImpl::SetEnableConsoleInspect(bool enable,
                                                          int view_id) {
  if (vm_type_ == kKeyEngineQuickjs) {
    auto sp = client_wp_.lock();
    CHECK_NULL_AND_LOG_RETURN(sp, "js debug: client is null");
    sp->SetEnableConsoleInspect(enable, view_id);
  }
}

std::string InspectorClientDelegateImpl::PrepareDispatchMessage(
    rapidjson::Document& message, int instance_id) {
  RemoveInvalidMembers(message);

  CacheBreakpointsByRequestMessage(message,
                                   GetScriptManagerByViewId(instance_id));
  RecordDebuggingInstanceID(message, instance_id);

  return base::ToJson(message);
}

std::string InspectorClientDelegateImpl::PrepareResponseMessage(
    const std::string& message, int instance_id) {
  std::string res;
  rapidjson::Document json_mes;
  if (!ParseStrToJson(json_mes, message)) {
    return res;
  }

  if (json_mes.HasMember(kKeyId) && json_mes[kKeyId].GetInt() <= 0) {
    int message_id = json_mes[kKeyId].GetInt();
    if (message_id < 0 && vm_type_ == kKeyEngineQuickjs &&
        json_mes.HasMember(kKeyResult) &&
        json_mes[kKeyResult].HasMember(kKeyResult) &&
        json_mes[kKeyResult][kKeyResult].HasMember(kKeyValue)) {
      std::string value =
          json_mes[kKeyResult][kKeyResult][kKeyValue].GetString();
      auto debugger = GetDebuggerByViewId(instance_id).lock();
      if (debugger != nullptr) {
        auto js_debugger =
            std::static_pointer_cast<InspectorJavaScriptDebuggerImpl>(debugger);
        js_debugger->OnConsoleObject(value, message_id);
      }
    }
    return res;
  } else if (json_mes.HasMember(kKeyMethod) &&
             strcmp(json_mes[kKeyMethod].GetString(),
                    kEventDebuggerScriptParsed) == 0) {
    int script_id = std::atoi(json_mes[kKeyParams][kKeyScriptId].GetString());
    std::string script_url = json_mes[kKeyParams][kKeyUrl].GetString();
    if (vm_type_ == kKeyEngineV8) {
      int script_view_id = GetScriptViewId(script_url);
      InsertScriptId(script_view_id, script_id);
    }
    if (script_url.empty() || IsScriptIdInvalid(script_id)) {
      return res;
    }
  } else if (json_mes.HasMember(kKeyMethod) &&
             std::strcmp(json_mes[kKeyMethod].GetString(),
                         kEventRuntimeConsoleAPICalled) == 0) {
    if (HandleMessageConsoleAPICalled(json_mes)) {
      return res;
    }
  }
  CacheBreakpointsByResponseMessage(json_mes,
                                    GetScriptManagerByViewId(instance_id));

  AddEngineTypeParam(json_mes);
  if (json_mes.HasMember(kKeyParams)) {
    json_mes[kKeyParams].AddMember(
        rapidjson::Value(kKeyViewId, json_mes.GetAllocator()),
        rapidjson::Value(instance_id), json_mes.GetAllocator());
  }

  // Add "sessionId" property if target_id_ is not empty.
  if (!target_id_.empty()) {
    json_mes.AddMember(rapidjson::Value(kKeySessionId, json_mes.GetAllocator()),
                       rapidjson::Value(target_id_, json_mes.GetAllocator()),
                       json_mes.GetAllocator());
  }

  res = base::ToJson(json_mes);
  return res;
}

bool InspectorClientDelegateImpl::HandleMessageConsoleAPICalled(
    rapidjson::Document& message) {
  if (vm_type_ == kKeyEngineV8) {
    return HandleMessageConsoleAPICalledFromV8(message);
  } else if (vm_type_ == kKeyEngineQuickjs) {
    HandleMessageConsoleAPICalledFromQuickjs(message);
  } else if (vm_type_ == kKeyEngineLepus) {
    HandleMessageConsoleAPICalledFromLepus(message);
  }
  return false;
}

bool InspectorClientDelegateImpl::HandleMessageConsoleAPICalledFromV8(
    rapidjson::Document& message) {
  if (!message.HasMember(kKeyParams) ||
      !message[kKeyParams].HasMember(kKeyArgs)) {
    return false;
  }
  auto& args = message[kKeyParams][kKeyArgs];
  if (args.IsArray() && args.GetArray().Size() > 1 &&
      args[0].HasMember(kKeyType) &&
      strcmp(args[0][kKeyType].GetString(), kKeyStringType) == 0) {
    std::string value = args[0][kKeyValue].GetString();
    int64_t runtime_id = kErrorViewID;
    std::string console_group_id = kErrorGroupStr;

    if (value.find(kKeyLepusRuntimeId) != std::string::npos) {
      runtime_id = std::stoi(value.substr(strlen(kKeyLepusRuntimeId) + 1));
      message[kKeyParams].AddMember(
          rapidjson::Value(kKeyConsoleTag, message.GetAllocator()),
          rapidjson::Value(kKeyEngineLepus, message.GetAllocator()),
          message.GetAllocator());
    } else if (value.find(kKeyRuntimeId) != std::string::npos) {
      runtime_id = std::stoi(value.substr(strlen(kKeyRuntimeId) + 1));
    } else if (value.find(kKeyGroupId) != std::string::npos) {
      console_group_id = value.substr(strlen(kKeyGroupId) + 1);
    } else {
      return false;
    }
    message[kKeyParams][kKeyArgs].Erase(message[kKeyParams][kKeyArgs].Begin());

    if (runtime_id != kErrorViewID) {
      int console_id = GetViewIdByRuntimeId(runtime_id);
      if (console_id == kErrorViewID) {
        return true;
      }
      message[kKeyParams].AddMember(
          rapidjson::Value(kKeyConsoleId, message.GetAllocator()),
          rapidjson::Value(console_id), message.GetAllocator());
    } else if (console_group_id != kErrorGroupStr) {
      message[kKeyParams].AddMember(
          rapidjson::Value(kKeyGroupId, message.GetAllocator()),
          rapidjson::Value(console_group_id, message.GetAllocator()),
          message.GetAllocator());
    }
  }
  return false;
}

void InspectorClientDelegateImpl::HandleMessageConsoleAPICalledFromQuickjs(
    rapidjson::Document& message) {
  if (message.HasMember(kKeyParams) &&
      message[kKeyParams].HasMember(kKeyRuntimeId)) {
    int runtime_id = message[kKeyParams][kKeyRuntimeId].GetInt();
    int console_view_id = GetViewIdByRuntimeId(runtime_id);
    message[kKeyParams].RemoveMember(kKeyRuntimeId);
    message[kKeyParams].AddMember(
        rapidjson::Value(kKeyConsoleId, message.GetAllocator()),
        rapidjson::Value(console_view_id), message.GetAllocator());
  }
}

void InspectorClientDelegateImpl::HandleMessageConsoleAPICalledFromLepus(
    rapidjson::Document& message) {
  if (message.HasMember(kKeyParams)) {
    message[kKeyParams].AddMember(
        rapidjson::Value(kKeyConsoleTag, message.GetAllocator()),
        rapidjson::Value(kKeyEngineLepus, message.GetAllocator()),
        message.GetAllocator());
    message[kKeyParams].AddMember(
        rapidjson::Value(kKeyConsoleId, message.GetAllocator()),
        rapidjson::Value(kDefaultViewID), message.GetAllocator());
  }
}

std::string InspectorClientDelegateImpl::GenMessageCallFunctionOn(
    const std::string& object_id, int message_id) {
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(
      rapidjson::Value(kKeyMethod, document.GetAllocator()),
      rapidjson::Value(kMethodRuntimeCallFunctionOn, document.GetAllocator()),
      document.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  params.AddMember(
      rapidjson::Value(kKeyFunctionDeclaration, params.GetAllocator()),
      rapidjson::Value(kStringifyObjectScript, params.GetAllocator()),
      params.GetAllocator());
  params.AddMember(rapidjson::Value(kKeyObjectId, params.GetAllocator()),
                   rapidjson::Value(object_id, params.GetAllocator()),
                   params.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyParams, document.GetAllocator()),
                     params, document.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyId, document.GetAllocator()),
                     rapidjson::Value(message_id), document.GetAllocator());
  return base::ToJson(document);
}

void InspectorClientDelegateImpl::SendMessageContextDestroyed(int view_id,
                                                              int context_id) {
  auto debugger = GetDebuggerByViewId(view_id).lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(rapidjson::Value(kKeyMethod, document.GetAllocator()),
                     rapidjson::Value(kEventRuntimeExecutionContextDestroyed,
                                      document.GetAllocator()),
                     document.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  params.AddMember(
      rapidjson::Value(kKeyExecutionContextId, params.GetAllocator()),
      rapidjson::Value(context_id), params.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyParams, document.GetAllocator()),
                     params, document.GetAllocator());
  debugger->SendResponse(base::ToJson(document));
}

void InspectorClientDelegateImpl::SendMessageContextCleared(int view_id) {
  auto debugger = GetDebuggerByViewId(view_id).lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "js debug: debugger is null");
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(rapidjson::Value(kKeyMethod, document.GetAllocator()),
                     rapidjson::Value(kEventRuntimeExecutionContextsCleared,
                                      document.GetAllocator()),
                     document.GetAllocator());
  debugger->SendResponse(base::ToJson(document));
}

void InspectorClientDelegateImpl::SendMessageRemoveScripts(int view_id) {
  auto it = view_id_to_bundle_.find(view_id);
  if (it != view_id_to_bundle_.end()) {
    auto& group_id = it->second.group_id_;
    auto view_set = GetViewIdInGroup(group_id);
    for (auto& id : view_set) {
      auto debugger = GetDebuggerByViewId(id).lock();
      if (debugger != nullptr) {
        rapidjson::Document document(rapidjson::kObjectType);
        document.AddMember(
            rapidjson::Value(kKeyMethod, document.GetAllocator()),
            rapidjson::Value(kEventDebuggerRemoveScriptsForLynxView,
                             document.GetAllocator()),
            document.GetAllocator());
        rapidjson::Document params(rapidjson::kObjectType);
        params.AddMember(rapidjson::Value(kKeyViewId, params.GetAllocator()),
                         rapidjson::Value(view_id), params.GetAllocator());
        document.AddMember(
            rapidjson::Value(kKeyParams, document.GetAllocator()), params,
            document.GetAllocator());
        debugger->SendResponse(base::ToJson(document));
      }
    }
  }
}

int InspectorClientDelegateImpl::GetScriptViewId(
    const std::string& script_url) {
  std::string url = script_url;
  size_t pos = url.find(kScriptUrlPrefix);
  if (pos != std::string::npos) {
    url = url.substr(std::strlen(kScriptUrlPrefix));
    pos = url.find("/");
    if (pos != std::string::npos) {
      int view_id = std::stoi(url.substr(0, pos));
      return view_id;
    }
  }
  return kErrorViewID;
}

}  // namespace devtool
}  // namespace lynx

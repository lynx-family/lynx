// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/js_inspect/inspector_client_delegate_base_impl.h"

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/timer/time_utils.h"
#include "core/base/json/json_util.h"

namespace lynx {
namespace devtool {

InspectorClientDelegateBaseImpl::InspectorClientDelegateBaseImpl(
    const std::string &vm_type)
    : vm_type_(vm_type) {}

void InspectorClientDelegateBaseImpl::DispatchMessageAsync(
    const std::string &message, int instance_id) {
  rapidjson::Document json_mes;
  if (!ParseStrToJson(json_mes, message)) {
    return;
  }

  auto result = PrepareDispatchMessage(json_mes, instance_id);

  std::unique_lock<std::mutex> lock(mutex_);
  DispatchMessageAsyncWithLockHeld(result, instance_id);
}

void InspectorClientDelegateBaseImpl::RunMessageLoopOnPause(
    const std::string &group_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (paused_) {
    return;
  }
  paused_ = true;
  cv_.wait(lock, [this] {
    FlushMessageQueueWithLockHeld();
    return !this->paused_;
  });
}

void InspectorClientDelegateBaseImpl::QuitMessageLoopOnPause() {
  std::lock_guard<std::mutex> lock(mutex_);
  paused_ = false;
}

double InspectorClientDelegateBaseImpl::CurrentTimeMS() {
  return static_cast<double>(base::CurrentTimeMilliseconds());
}

void InspectorClientDelegateBaseImpl::StartRepeatingTimer(
    double interval, std::function<void(void *)> callback, void *data) {
  if (timer_ == nullptr) {
    // Since message_loop.cc may be compiled in different dynamic libraries,
    // which will cause there are different static variables
    // 'tls_message_loop_instance' (in message_loop.cc). When constructing the
    // TimedTaskManager, MessageLoop::GetCurrent() may get a nullptr. So we need
    // to call EnsureInitializedForCurrentThread(), it will initialize a new
    // MessageLoop and TaskRunner but also run on the current (JS) thread.
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    timer_ = std::make_unique<base::TimedTaskManager>();
  }
  uint32_t task_id = timer_->SetInterval([callback, data]() { callback(data); },
                                         static_cast<int64_t>(interval * 1000));
  timed_task_ids_.emplace(data, task_id);
}

void InspectorClientDelegateBaseImpl::CancelTimer(void *data) {
  if (timer_ == nullptr) {
    timer_ = std::make_unique<base::TimedTaskManager>();
  }
  auto iter = timed_task_ids_.find(data);
  if (iter != timed_task_ids_.end()) {
    timer_->StopTask(iter->second);
    timed_task_ids_.erase(iter);
  }
}

void InspectorClientDelegateBaseImpl::StopDebug(int instance_id) {
  if (debugging_instance_id_ == instance_id) {
    // Since we ensure that only one session is enabled at the same time,
    // sending a Debugger.disable message to the JS engine can quit the paused
    // state and will not trigger subsequent breakpoints.
    DispatchMessageAsync(GenSimpleMessage(kMethodDebuggerDisable), instance_id);
  }
}

void InspectorClientDelegateBaseImpl::DispatchMessageAsyncWithLockHeld(
    const std::string &message, int instance_id) {
  message_queue_.emplace(std::make_pair(instance_id, message));
  if (paused_) {
    cv_.notify_all();
  } else {
    std::shared_ptr<InspectorClientDelegateBaseImpl> ref = shared_from_this();
    PostTask(instance_id, [ref] { ref->FlushMessageQueue(); });
  }
}

void InspectorClientDelegateBaseImpl::FlushMessageQueue() {
  std::unique_lock<std::mutex> lock(mutex_);
  FlushMessageQueueWithLockHeld();
}

void InspectorClientDelegateBaseImpl::FlushMessageQueueWithLockHeld() {
  while (!message_queue_.empty()) {
    int instance_id = message_queue_.front().first;
    std::string mes = message_queue_.front().second;
    message_queue_.pop();
    mutex_.unlock();  // Unlock to dispatch message to js engine.
    DispatchMessage(mes, instance_id);
    mutex_.lock();
  }
}

void InspectorClientDelegateBaseImpl::DispatchInitMessage(
    int instance_id, const std::unique_ptr<ScriptManagerNG> &script_manager,
    bool runtime_enable) {
  DispatchMessage(GenSimpleMessage(kMethodDebuggerEnable), instance_id);
  if (runtime_enable) {
    DispatchMessage(GenSimpleMessage(kMethodRuntimeEnable), instance_id);
  }
  DispatchMessage(GenSimpleMessage(kMethodProfilerEnable), instance_id);
  SetBreakpointCached(instance_id, script_manager);
}

void InspectorClientDelegateBaseImpl::SetBreakpointCached(
    int instance_id, const std::unique_ptr<ScriptManagerNG> &script_manager) {
  if (script_manager == nullptr) {
    return;
  }
  auto &breakpoint = script_manager->GetBreakpoints();
  if (breakpoint.empty()) {
    return;
  }
  for (const auto &bp : breakpoint) {
    auto mes = GenMessageSetBreakpointByUrl(
        bp.second.url_, bp.second.condition_, bp.second.line_number_,
        bp.second.column_number_);
    DispatchMessage(mes, instance_id);
  }
  auto active_mes =
      GenMessageSetBreakpointsActive(script_manager->GetBreakpointsActive());
  DispatchMessage(active_mes, instance_id);
}

std::string InspectorClientDelegateBaseImpl::PrepareDispatchMessage(
    rapidjson::Document &message, int instance_id) {
  /*
   * We provide a template here.
   *
   * RemoveInvalidMembers(message);
   * CacheBreakpointsByRequestMessage(message, script_manager);
   * RecordDebuggingInstanceID(message, instance_id);
   * // Do something else...
   *
   * return base::ToJson(message);
   */
  RemoveInvalidMembers(message);
  return base::ToJson(message);
}

std::string InspectorClientDelegateBaseImpl::PrepareResponseMessage(
    const std::string &message, int instance_id) {
  /*
   * We provide a template here.
   *
   * std::string res;
   * rapidjson::Document json_mes;
   * if (!ParseStrToJson(json_mes, message)) {
   *   return res;
   * }
   *
   * CacheBreakpointsByResponseMessage(json_mes, script_manager);
   * AddEngineTypeParam(json_mes);
   * // Do something else...
   *
   * res = base::ToJson(json_mes);
   * return res;
   */
  std::string res;
  rapidjson::Document json_mes;
  if (!ParseStrToJson(json_mes, message)) {
    return res;
  }
  res = base::ToJson(json_mes);
  return res;
}

void InspectorClientDelegateBaseImpl::CacheBreakpointsByRequestMessage(
    const rapidjson::Document &message,
    const std::unique_ptr<ScriptManagerNG> &script_manager) {
  if (script_manager == nullptr) {
    return;
  }
  std::string method = message[kKeyMethod].GetString();
  if (method == kMethodDebuggerSetBreakpointsActive) {
    script_manager->SetBreakpointsActive(
        message[kKeyParams][kKeyActive].GetBool());
  } else if (method == kMethodDebuggerSetBreakpointByUrl) {
    script_manager->SetBreakpointDetail(message);
  } else if (method == kMethodDebuggerRemoveBreakpoint) {
    script_manager->RemoveBreakpoint(
        message[kKeyParams][kKeyBreakpointId].GetString());
  } else if (method == kMethodDebuggerEnable) {
    if (!script_manager->GetBreakpointsActive()) {
      script_manager->SetBreakpointsActive(true);
    }
  }
}

void InspectorClientDelegateBaseImpl::CacheBreakpointsByResponseMessage(
    const rapidjson::Document &message,
    const std::unique_ptr<ScriptManagerNG> &script_manager) {
  if (message.HasMember(kKeyId) && script_manager != nullptr) {
    script_manager->SetBreakpointId(message);
  }
}

void InspectorClientDelegateBaseImpl::RecordDebuggingInstanceID(
    const rapidjson::Document &message, int instance_id) {
  std::string method = message[kKeyMethod].GetString();
  if (method == kMethodDebuggerEnable) {
    debugging_instance_id_ = instance_id;
  } else if (method == kMethodDebuggerDisable &&
             debugging_instance_id_ == instance_id) {
    debugging_instance_id_ = kErrorViewID;
  }
}

void InspectorClientDelegateBaseImpl::AddEngineTypeParam(
    rapidjson::Document &message) {
  if (message.HasMember(kKeyResult) &&
      message[kKeyResult].HasMember(kKeyDebuggerId) &&
      vm_type_ != kKeyEngineLepus) {
    message[kKeyResult].AddMember(
        rapidjson::Value(kKeyEngineType, message.GetAllocator()),
        rapidjson::Value(vm_type_, message.GetAllocator()),
        message.GetAllocator());
  }
}

std::string InspectorClientDelegateBaseImpl::GenSimpleMessage(
    const std::string &method, int message_id) {
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(rapidjson::Value(kKeyId, document.GetAllocator()),
                     rapidjson::Value(message_id), document.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyMethod, document.GetAllocator()),
                     rapidjson::Value(method, document.GetAllocator()),
                     document.GetAllocator());
  return base::ToJson(document);
}

std::string InspectorClientDelegateBaseImpl::GenMessageSetBreakpointByUrl(
    const std::string &url, const std::string &condition, int line, int column,
    int message_id) {
  rapidjson::Document content(rapidjson::kObjectType);
  content.AddMember(rapidjson::Value(kKeyId, content.GetAllocator()),
                    rapidjson::Value(message_id), content.GetAllocator());
  content.AddMember(rapidjson::Value(kKeyMethod, content.GetAllocator()),
                    rapidjson::Value(kMethodDebuggerSetBreakpointByUrl,
                                     content.GetAllocator()),
                    content.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  params.AddMember(rapidjson::Value(kKeyUrl, params.GetAllocator()),
                   rapidjson::Value(url, params.GetAllocator()),
                   params.GetAllocator());
  if (!condition.empty()) {
    params.AddMember(rapidjson::Value(kKeyCondition, params.GetAllocator()),
                     rapidjson::Value(condition, params.GetAllocator()),
                     params.GetAllocator());
  }
  params.AddMember(rapidjson::Value(kKeyLineNumber, params.GetAllocator()),
                   rapidjson::Value(line), params.GetAllocator());
  params.AddMember(rapidjson::Value(kKeyColumnNumber, params.GetAllocator()),
                   rapidjson::Value(column), params.GetAllocator());
  content.AddMember(rapidjson::Value(kKeyParams, content.GetAllocator()),
                    params, content.GetAllocator());
  return base::ToJson(content);
}

std::string InspectorClientDelegateBaseImpl::GenMessageSetBreakpointsActive(
    bool active, int message_id) {
  rapidjson::Document content(rapidjson::kObjectType);
  content.AddMember(rapidjson::Value(kKeyId, content.GetAllocator()),
                    rapidjson::Value(message_id), content.GetAllocator());
  content.AddMember(rapidjson::Value(kKeyMethod, content.GetAllocator()),
                    rapidjson::Value(kMethodDebuggerSetBreakpointsActive,
                                     content.GetAllocator()),
                    content.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  params.AddMember(rapidjson::Value(kKeyActive, params.GetAllocator()),
                   rapidjson::Value(active), params.GetAllocator());
  content.AddMember(rapidjson::Value(kKeyParams, content.GetAllocator()),
                    params, content.GetAllocator());
  return base::ToJson(content);
}

rapidjson::Document InspectorClientDelegateBaseImpl::GenTargetInfo(
    const std::string &target_id, const std::string &title) {
  rapidjson::Document info(rapidjson::kObjectType);
  info.AddMember(rapidjson::Value(kKeyTargetId, info.GetAllocator()),
                 rapidjson::Value(target_id, info.GetAllocator()),
                 info.GetAllocator());
  info.AddMember(rapidjson::Value(kKeyType, info.GetAllocator()),
                 rapidjson::Value(kKeyTypeWorker, info.GetAllocator()),
                 info.GetAllocator());
  info.AddMember(rapidjson::Value(kKeyTitle, info.GetAllocator()),
                 rapidjson::Value(title, info.GetAllocator()),
                 info.GetAllocator());
  info.AddMember(rapidjson::Value(kKeyUrl, info.GetAllocator()),
                 rapidjson::Value("", info.GetAllocator()),
                 info.GetAllocator());
  info.AddMember(rapidjson::Value(kKeyAttached, info.GetAllocator()),
                 rapidjson::Value(false), info.GetAllocator());
  info.AddMember(rapidjson::Value(kKeyCanAccessOpener, info.GetAllocator()),
                 rapidjson::Value(false), info.GetAllocator());
  return info;
}

std::string InspectorClientDelegateBaseImpl::GenMessageTargetCreated(
    const std::string &target_id, const std::string &title) {
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(
      rapidjson::Value(kKeyMethod, document.GetAllocator()),
      rapidjson::Value(kEventTargetCreated, document.GetAllocator()),
      document.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  auto info = GenTargetInfo(target_id, title);
  params.AddMember(rapidjson::Value(kKeyTargetInfo, params.GetAllocator()),
                   info, params.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyParams, document.GetAllocator()),
                     params, document.GetAllocator());
  return base::ToJson(document);
}

std::string InspectorClientDelegateBaseImpl::GenMessageAttachedToTarget(
    const std::string &target_id, const std::string &session_id,
    const std::string &title) {
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(
      rapidjson::Value(kKeyMethod, document.GetAllocator()),
      rapidjson::Value(kEventAttachedToTarget, document.GetAllocator()),
      document.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  auto info = GenTargetInfo(target_id, title);
  info[kKeyAttached] = true;
  params.AddMember(rapidjson::Value(kKeySessionId, params.GetAllocator()),
                   rapidjson::Value(session_id, params.GetAllocator()),
                   params.GetAllocator());
  params.AddMember(rapidjson::Value(kKeyTargetInfo, params.GetAllocator()),
                   info, params.GetAllocator());
  params.AddMember(
      rapidjson::Value(kKeyWaitingForDebugger, params.GetAllocator()),
      rapidjson::Value(true), params.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyParams, document.GetAllocator()),
                     params, document.GetAllocator());
  return base::ToJson(document);
}

std::string InspectorClientDelegateBaseImpl::GenMessageTargetDestroyed(
    const std::string &target_id) {
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(
      rapidjson::Value(kKeyMethod, document.GetAllocator()),
      rapidjson::Value(kEventTargetDestroyed, document.GetAllocator()),
      document.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  params.AddMember(rapidjson::Value(kKeyTargetId, params.GetAllocator()),
                   rapidjson::Value(target_id, params.GetAllocator()),
                   params.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyParams, document.GetAllocator()),
                     params, document.GetAllocator());
  return base::ToJson(document);
}

std::string InspectorClientDelegateBaseImpl::GenMessageDetachedFromTarget(
    const std::string &session_id) {
  rapidjson::Document document(rapidjson::kObjectType);
  document.AddMember(
      rapidjson::Value(kKeyMethod, document.GetAllocator()),
      rapidjson::Value(kEventDetachedFromTarget, document.GetAllocator()),
      document.GetAllocator());
  rapidjson::Document params(rapidjson::kObjectType);
  params.AddMember(rapidjson::Value(kKeySessionId, params.GetAllocator()),
                   rapidjson::Value(session_id, params.GetAllocator()),
                   params.GetAllocator());
  document.AddMember(rapidjson::Value(kKeyParams, document.GetAllocator()),
                     params, document.GetAllocator());
  return base::ToJson(document);
}

bool InspectorClientDelegateBaseImpl::ParseStrToJson(
    rapidjson::Document &json_mes, const std::string &mes) {
  json_mes = base::strToJson(mes.c_str());
  if (json_mes.HasParseError()) {
    LOGE("js debug: parse json str error! original str: " << mes);
    return false;
  }
  return true;
}

void InspectorClientDelegateBaseImpl::RemoveInvalidMembers(
    rapidjson::Value &message) {
  // V8 can only process CDP messages with the following members:
  // "id", "method", "params" and "sessionId"
  // If there are some other members in a CDP message, then the message won't be
  // processed, so we need to remove them.
  for (auto it = message.MemberBegin(); it != message.MemberEnd();) {
    auto key = it->name.GetString();
    if (strcmp(key, kKeyId) && strcmp(key, kKeyMethod) &&
        strcmp(key, kKeyParams) && strcmp(key, kKeySessionId)) {
      it = message.EraseMember(it);
    } else {
      it++;
    }
  }
}

}  // namespace devtool
}  // namespace lynx

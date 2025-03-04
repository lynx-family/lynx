// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/v8/v8_inspector_client_impl.h"

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/include/string/string_utils.h"
#include "devtool/fundamentals/js_inspect/inspector_client_delegate.h"
#include "devtool/js_inspect/inspector_const.h"

namespace lynx {
namespace devtool {

namespace {

std::string StringViewToUtf8(const v8_inspector::StringView& view) {
  if (view.length() == 0) return "";
  if (view.is8Bit())
    return std::string(reinterpret_cast<const char*>(view.characters8()),
                       view.length());
  const uint16_t* source = view.characters16();
  const char16_t* ch = reinterpret_cast<const char16_t*>(source);
  std::u16string utf16(ch);
  return lynx::base::U16StringToU8(utf16);
}

ALLOW_UNUSED_TYPE v8_inspector::StringView Utf8ToStringView(
    const std::string& message) {
  v8_inspector::StringView message_view(
      reinterpret_cast<const uint8_t*>(message.c_str()), message.size());
  return message_view;
}

int GenerateGroupId() {
  static base::NoDestructor<std::atomic<int>> id{1};
  return (*id)++;
}

}  // namespace

// V8ChannelImpl begins.
V8ChannelImpl::V8ChannelImpl(
    const std::unique_ptr<v8_inspector::V8Inspector>& inspector,
    const std::shared_ptr<V8InspectorClientImpl>& client, int instance_id,
    int group_id)
    : client_wp_(client), instance_id_(instance_id), group_id_(group_id) {
#if V8_MAJOR_VERSION > 10 ||   \
    (V8_MAJOR_VERSION == 10 && \
     (V8_MINOR_VERSION > 3 ||  \
      V8_MINOR_VERSION == 3 && V8_BUILD_NUMBER >= 118))
  session_ = inspector->connect(group_id_, this, v8_inspector::StringView(),
                                v8_inspector::V8Inspector::kFullyTrusted);
#else
  session_ = inspector->connect(group_id_, this, v8_inspector::StringView());
#endif
}

void V8ChannelImpl::sendResponse(
    int callId, std::unique_ptr<v8_inspector::StringBuffer> message) {
  SendResponseToClient(message);
}

void V8ChannelImpl::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  SendResponseToClient(message);
}

void V8ChannelImpl::DispatchProtocolMessage(const std::string& message) {
  v8_inspector::StringView message_view = Utf8ToStringView(message);
  if (session_ != nullptr) {
    session_->dispatchProtocolMessage(message_view);
  }
}

void V8ChannelImpl::SchedulePauseOnNextStatement(const std::string& reason) {
  v8_inspector::StringView message_view = Utf8ToStringView(reason);
  if (session_ != nullptr) {
    session_->schedulePauseOnNextStatement(message_view, message_view);
  }
}

void V8ChannelImpl::CancelPauseOnNextStatement() {
  if (session_ != nullptr) {
    session_->cancelPauseOnNextStatement();
  }
}

void V8ChannelImpl::SendResponseToClient(
    const std::unique_ptr<v8_inspector::StringBuffer>& message) {
  v8_inspector::StringView message_view = message->string();
  std::string str = StringViewToUtf8(message_view);

  auto sp = client_wp_.lock();
  if (sp != nullptr) {
    sp->SendResponse(str, instance_id_);
  }
}
// V8ChannelImpl ends.

// V8InspectorClientImpl begins.
void V8InspectorClientImpl::runMessageLoopOnPause(int contextGroupId) {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->RunMessageLoopOnPause(std::to_string(contextGroupId));
  }
}

void V8InspectorClientImpl::quitMessageLoopOnPause() {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->QuitMessageLoopOnPause();
  }
}

v8::Local<v8::Context> V8InspectorClientImpl::ensureDefaultContextInGroup(
    int contextGroupId) {
  auto it = contexts_.find(contextGroupId);
  if (it != contexts_.end()) {
    return it->second.Get(isolate_);
  }
  return v8::Local<v8::Context>();
}

double V8InspectorClientImpl::currentTimeMS() {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    return sp->CurrentTimeMS();
  }
  return 0;
}

void V8InspectorClientImpl::startRepeatingTimer(double interval,
                                                TimerCallback callback,
                                                void* data) {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->StartRepeatingTimer(interval, callback, data);
  }
}

void V8InspectorClientImpl::cancelTimer(void* data) {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->CancelTimer(data);
  }
}

void V8InspectorClientImpl::SetStopAtEntry(bool stop_at_entry,
                                           int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
    if (stop_at_entry) {
      it->second->SchedulePauseOnNextStatement(kStopAtEntryReason);
    } else {
      it->second->CancelPauseOnNextStatement();
    }
  }
}

void V8InspectorClientImpl::DispatchMessage(const std::string& message,
                                            int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
#if V8_MAJOR_VERSION >= 9
    v8::Isolate::Scope scope(isolate_);
#endif
    it->second->DispatchProtocolMessage(message);
  }
}

int V8InspectorClientImpl::InitInspector(v8::Isolate* isolate,
                                         v8::Local<v8::Context> ctx,
                                         const std::string& group_id,
                                         const std::string& name) {
  LOGI("js debug: V8InspectorClientImpl::InitInspector, this: " << this);
  isolate_ = isolate;
  int group_num = MapGroupStrToNum(group_id);
  CreateV8Inspector();
  ContextCreated(ctx, group_num, name);
  return group_num;
}

void V8InspectorClientImpl::ConnectSession(int instance_id, int group_id) {
  if (channels_.find(instance_id) == channels_.end()) {
    channels_.emplace(
        instance_id,
        std::make_shared<V8ChannelImpl>(
            inspector_,
            std::static_pointer_cast<V8InspectorClientImpl>(shared_from_this()),
            instance_id, group_id));
  }
}

void V8InspectorClientImpl::DisconnectSession(int instance_id) {
  auto it = channels_.find(instance_id);
  if (it != channels_.end()) {
    int group_id = it->second->GroupId();
    channels_.erase(it);
    auto sp = delegate_wp_.lock();
    if (sp != nullptr) {
      sp->OnSessionDestroyed(instance_id, std::to_string(group_id));
    }
  }
}

void V8InspectorClientImpl::DestroyContext(int group_id) {
  ContextDestroyed(group_id);
}

void V8InspectorClientImpl::DestroyContext(const std::string& group_id) {
  auto it = group_string_to_number_.find(group_id);
  if (it == group_string_to_number_.end()) {
    return;
  }
  ContextDestroyed(it->second);
}

void V8InspectorClientImpl::DestroyInspector() {
  // Must destroy all Channels before destroying V8Inspector.
  DestroyAllSessions();
  inspector_.reset();
  isolate_ = nullptr;
}

void V8InspectorClientImpl::CreateV8Inspector() {
  if (inspector_ == nullptr) {
    inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  }
}

void V8InspectorClientImpl::ContextCreated(v8::Local<v8::Context> context,
                                           int group_id,
                                           const std::string& name) {
  if (contexts_.find(group_id) != contexts_.end()) {
    return;
  }
  contexts_.emplace(
      group_id,
      v8::Persistent<v8::Context, v8::CopyablePersistentTraits<v8::Context>>(
          isolate_, context));
  v8_inspector::V8ContextInfo info(context, group_id, Utf8ToStringView(name));
  inspector_->contextCreated(info);
  LOGI("js debug: ContextCreated, group_id: " << group_id);
}

void V8InspectorClientImpl::ContextDestroyed(int group_id) {
  auto it = contexts_.find(group_id);
  if (it == contexts_.end()) {
    LOGI(
        "js debug: V8InspectorClientImpl::ContextDestroyed, cannot find the "
        "context with the specific group_id!");
    return;
  }
  LOGI("js debug: V8InspectorClientImpl::ContextDestroyed, context IsEmpty: "
       << it->second.IsEmpty() << ", inspector: " << inspector_);
  if (isolate_ != nullptr && inspector_ != nullptr && !it->second.IsEmpty()) {
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    auto context = it->second.Get(isolate_);
    v8::Context::Scope context_scope(context);
    int context_id = v8_inspector::V8ContextInfo::executionContextId(context);
    inspector_->contextDestroyed(context);
    inspector_->resetContextGroup(group_id);
    it->second.Reset();
    contexts_.erase(it);
    RemoveGroupMapping(group_id);
    auto sp = delegate_wp_.lock();
    if (sp != nullptr) {
      sp->OnContextDestroyed(std::to_string(group_id), context_id);
    }
  }
}

void V8InspectorClientImpl::DestroyAllSessions() {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    for (auto& item : channels_) {
      sp->OnSessionDestroyed(item.first,
                             std::to_string(item.second->GroupId()));
    }
  }
  channels_.clear();
}

int V8InspectorClientImpl::MapGroupStrToNum(const std::string& group_string) {
  if (group_string == kSingleGroupStr) {
    return GenerateGroupId();
  } else {
    auto it = group_string_to_number_.find(group_string);
    if (it == group_string_to_number_.end()) {
      int group_num = GenerateGroupId();
      group_string_to_number_[group_string] = group_num;
      group_number_to_string_[group_num] = group_string;
    }
    return group_string_to_number_[group_string];
  }
}

void V8InspectorClientImpl::RemoveGroupMapping(int group_num) {
  auto it = group_number_to_string_.find(group_num);
  if (it != group_number_to_string_.end()) {
    group_string_to_number_.erase(it->second);
    group_number_to_string_.erase(it);
  }
}
// V8InspectorClientImpl ends.

}  // namespace devtool
}  // namespace lynx

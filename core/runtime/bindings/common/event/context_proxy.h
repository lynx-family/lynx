// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_EVENT_CONTEXT_PROXY_H_
#define CORE_RUNTIME_BINDINGS_COMMON_EVENT_CONTEXT_PROXY_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "core/event/event_listener.h"
#include "core/event/event_target.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace runtime {

class MessageEvent;

// Currently, Lynx will have at least two contexts existing at the same time,
// JSContext and CoreContext. The need for Context Communication API is very
// reasonable for frontend frameworks. However, since these Contexts may not be
// on the same thread, we can't expand a real Context object, but can only
// provide a ContextProxy object to implement the Communication API.
class ContextProxy : public event::EventTarget {
 public:
  // When executing ConvertStringToContextType, kUnKnown is used to represent
  // that the string has no corresponding Context type. At the same time,
  // kUnKnown is also used to indicate the current supported count of
  // ContextProxy types.
  enum class Type : int32_t {
    kJSContext = 0,
    kCoreContext,
    kUIContext,
    kDevTool,
    kUnknown
  };

  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;
    virtual event::DispatchEventResult DispatchMessageEvent(
        MessageEvent event) = 0;
  };

  ContextProxy(Delegate& delegate, Type origin_type, Type target_type)
      : delegate_(delegate),
        origin_type_(origin_type),
        target_type_(target_type){};
  virtual ~ContextProxy(){};

  static std::string ConvertContextTypeToString(ContextProxy::Type type);
  static Type ConvertStringToContextType(const std::string& type_str);

  virtual void PostMessage(const lepus::Value& message);

  void SetListenerBeforePublishEvent(
      std::unique_ptr<event::EventListener> listener);
  event::EventListener* GetListenerBeforePublishEvent();

  virtual event::DispatchEventResult DispatchEvent(
      event::Event& event) override;

  virtual EventTarget* GetParentTarget() override { return nullptr; };

  Type GetTargetType() const { return target_type_; }
  Type GetOriginType() const { return origin_type_; }

 protected:
  Delegate& delegate_;
  Type origin_type_;
  Type target_type_;

  std::unique_ptr<event::EventListener> event_listener_{nullptr};
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_EVENT_CONTEXT_PROXY_H_

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_EVENTS_EVENTS_H_
#define CORE_RENDERER_EVENTS_EVENTS_H_
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

constexpr const static char* kEventBindEvent = "bindEvent";
constexpr const static char* kEventCatchEvent = "catchEvent";
constexpr const static char* kEventCaptureBind = "capture-bind";
constexpr const static char* kEventCaptureCatch = "capture-catch";
constexpr const static char* kEventGlobalBind = "global-bindEvent";

struct EventOption {
  // Determines whether the event can bubble. Default value is false.
  bool bubbles_{false};
  // Determines whether the event can cross the component boundary. When it is
  // false, the event will only be triggered on the node tree that references
  // the component, and will not enter any other components. Default value is
  // false.
  bool composed_{false};
  // Determines whether the event has a capture phase. Default value is false.
  bool capture_phase_{false};
  // Determines whether the event is listened by lepus.
  bool lepus_event_{false};
  // Determines whether the event is triggered by fe.
  bool from_frontend_{false};
};

struct PiperEventContent {
  // jsb event function name
  base::String piper_func_name_;
  // jsb event function args
  lepus::Value piper_func_args_;

  // Constructor
  // piper_func_name: jsb method name
  // piper_func_args: args needed for jsb method , the format:
  // {tasmEntryName:__Card__, callbackId:0, fromPiper:true, methodDetail:
  // {method:aMethod, module:aModule, param:[arg1, arg2, ...]}}
  PiperEventContent(const base::String piper_func_name,
                    const lepus::Value piper_func_args)
      : piper_func_name_(piper_func_name), piper_func_args_(piper_func_args) {}

  constexpr const static char kPiperFunctionName[] = "piperFunctionName";
  constexpr const static char kPiperFuncArgs[] = "piperFunctionParameters";
  lepus::Value ToLepusValue() const;
};

// https://developer.mozilla.org/en-US/docs/Web/API/Event/eventPhase
enum EventPhase {
  kNONE = 0,
  kCAPTURING_PHASE = 1,
  kAT_TARGET = 2,
  kBUBBLING_PHASE = 3
};

enum class EventTypeEnum {
  kBindEvent = 0,
  kCatchEvent = 1,
  kCaptureBind = 2,
  kCaptureCatch = 3,
  kGlobalBind = 4,
  kMax
};

class EventHandler {
 public:
  EventHandler(const base::String& type, const base::String& name,
               const base::String& function)
      : EventHandler(true, type, name, function, lepus::Value(), lepus::Value(),
                     lepus::Value(), std::nullopt, nullptr) {}

  EventHandler(const base::String& type, const base::String& name,
               const lepus::Value& lepus_script,
               const lepus::Value& lepus_function)
      : EventHandler(false, type, name, base::String(), lepus_script,
                     lepus_function, lepus::Value(), std::nullopt, nullptr) {}

  // Constructor for lepus events with object param, The main scenario is
  // element worklet in fiber.
  EventHandler(const base::String& type, const base::String& name,
               const lepus::Value& lepus_object, lepus::Context* context)
      : EventHandler(false, type, name, base::String(), lepus::Value(),
                     lepus::Value(), lepus_object, std::nullopt, context) {}

  // Constructor for SSR server events, supports multiply jsb calls.
  EventHandler(
      const base::String& type, const base::String& name,
      const std::optional<std::vector<PiperEventContent>>& piper_event_vec)
      : EventHandler(true, type, name, base::String(), lepus::Value(),
                     lepus::Value(), lepus::Value(), piper_event_vec, nullptr) {
  }

  EventHandler(const EventHandler& other) {
    this->is_js_event_ = other.is_js_event_;
    this->type_ = other.type_;
    this->name_ = other.name_;
    this->function_ = other.function_;
    this->lepus_script_ = other.lepus_script_;
    this->lepus_function_ = other.lepus_function_;
    this->piper_event_vec_ = other.piper_event_vec_;
  }
  virtual ~EventHandler() {}

  EventHandler& operator=(const EventHandler& other) {
    this->is_js_event_ = other.is_js_event_;
    this->type_ = other.type_;
    this->name_ = other.name_;
    this->function_ = other.function_;
    this->lepus_script_ = other.lepus_script_;
    this->lepus_function_ = other.lepus_function_;
    this->piper_event_vec_ = other.piper_event_vec_;
    return *this;
  }

  bool is_js_event() const { return is_js_event_; }
  bool is_piper_event() const { return piper_event_vec_.has_value(); }
  const base::String& name() const { return name_; }
  const base::String& type() const { return type_; }
  const base::String& function() const { return function_; }

  const lepus::Value& lepus_script() const { return lepus_script_; }
  const lepus::Value& lepus_function() const { return lepus_function_; }
  lepus::Value& lepus_function() { return lepus_function_; }

  // lepus object, the main usage scenario is worklet in fiber
  const lepus::Value& lepus_object() { return lepus_object_; }

  // lepus context, the main usage scenario is worklet in fiber
  lepus::Context* lepus_context() { return ctx_; }

  std::optional<std::vector<PiperEventContent>>& piper_event_vec() {
    return piper_event_vec_;
  }

  virtual bool IsBindEvent() const;
  virtual bool IsCatchEvent() const;
  virtual bool IsCaptureBindEvent() const;
  virtual bool IsCaptureCatchEvent() const;
  virtual bool IsGlobalBindEvent() const;

  lepus::Value ToLepusValue() const;
  PubLepusValue ToPubLepusValue() const;
  EventPhase GetEventPhase() const;

 private:
  EventHandler(
      bool is_js_event, const base::String& type, const base::String& name,
      const base::String& function, const lepus::Value& lepus_script,
      const lepus::Value& lepus_function, const lepus::Value& lepus_object,
      const std::optional<std::vector<PiperEventContent>>& piper_event_vec,
      lepus::Context* ctx)
      : is_js_event_(is_js_event),
        type_(type),
        name_(name),
        function_(function),
        lepus_script_(lepus_script),
        lepus_function_(lepus_function),
        lepus_object_(lepus_object),
        piper_event_vec_(piper_event_vec),
        ctx_(ctx) {}

  bool is_js_event_;
  base::String type_;
  base::String name_;
  // JS function name
  base::String function_;

  // lepus script, js object
  lepus::Value lepus_script_;
  // lepus function, js object
  lepus::Value lepus_function_;

  // lepus object, js object
  lepus::Value lepus_object_;

  // ssr server events vector
  std::optional<std::vector<PiperEventContent>> piper_event_vec_;

  // lepus context
  lepus::Context* ctx_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_EVENTS_EVENTS_H_

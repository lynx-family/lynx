// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/renderer/events/events.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

lepus::Value PiperEventContent::ToLepusValue() const {
  lepus::Value dict = lepus::Value(lepus::Dictionary::Create());
  dict.SetProperty(BASE_STATIC_STRING(kPiperFunctionName),
                   lepus::Value(piper_func_name_));
  dict.SetProperty(BASE_STATIC_STRING(kPiperFuncArgs), piper_func_args_);
  return dict;
}

bool EventHandler::IsBindEvent() const { return type_ == kEventBindEvent; }

bool EventHandler::IsCatchEvent() const { return type_ == kEventCatchEvent; }

bool EventHandler::IsCaptureBindEvent() const {
  return type_ == kEventCaptureBind;
}

bool EventHandler::IsCaptureCatchEvent() const {
  return type_ == kEventCaptureCatch;
}

bool EventHandler::IsGlobalBindEvent() const {
  return type_ == kEventGlobalBind;
}

// The return value contains name, type, jsFunction, lepusFunction and
// piperEventContent. It must contain name and type, and may contain only one of
// jsFunction, lepusFunction and piperEventContent.
lepus::Value EventHandler::ToLepusValue() const {
  constexpr const static char kEventName[] = "name";
  constexpr const static char kEventType[] = "type";
  constexpr const static char kFunctionName[] = "jsFunction";
  constexpr const static char kLepusFunction[] = "lepusFunction";
  constexpr const static char kPiperEventContent[] = "piperEventContent";

  lepus::Value dict = lepus::Value(lepus::Dictionary::Create());

  dict.SetProperty(BASE_STATIC_STRING(kEventName), lepus::Value(name_));
  dict.SetProperty(BASE_STATIC_STRING(kEventType), lepus::Value(type_));

  if (!function_.empty()) {
    dict.SetProperty(BASE_STATIC_STRING(kFunctionName),
                     lepus::Value(function_));
  }
  if (!lepus_function_.IsEmpty()) {
    dict.SetProperty(BASE_STATIC_STRING(kLepusFunction),
                     lepus::Value(lepus_function_));
  }
  if (piper_event_vec_.has_value() && !piper_event_vec_->empty()) {
    const auto& ary = lepus::CArray::Create();
    for (const auto& piper_event : *piper_event_vec_) {
      ary->emplace_back(piper_event.ToLepusValue());
    }
    dict.SetProperty(BASE_STATIC_STRING(kPiperEventContent),
                     lepus::Value(std::move(ary)));
  }

  return dict;
}

PubLepusValue EventHandler::ToPubLepusValue() const {
  auto array = lepus::CArray::Create();
  array->emplace_back(name_);
  array->emplace_back(type_);
  array->emplace_back(is_js_event_);
  array->emplace_back(function_);
  return PubLepusValue(lepus::Value(std::move(array)));
}

EventPhase EventHandler::GetEventPhase() const {
  if (IsBindEvent() || IsCatchEvent()) {
    return EventPhase::kBUBBLING_PHASE;
  }
  if (IsCaptureBindEvent() || IsCaptureCatchEvent()) {
    return EventPhase::kCAPTURING_PHASE;
  }
  return EventPhase::kNONE;
}

}  // namespace tasm
}  // namespace lynx

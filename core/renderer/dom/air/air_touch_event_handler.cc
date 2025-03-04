// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_touch_event_handler.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/services/replay/replay_controller.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

#define EVENT_TOUCH_START "touchstart"
#define EVENT_TOUCH_MOVE "touchmove"
#define EVENT_TOUCH_CANCEL "touchcancel"
#define EVENT_TOUCH_END "touchend"
#define EVENT_TAP "tap"
#define EVENT_LONG_PRESS "longpress"

// TODO(liukeang): after validation of Air Mode, merge code with
// touch_event_handler.cc
AirTouchEventHandler::AirTouchEventHandler(AirNodeManager *air_node_manager)
    : air_node_manager_(air_node_manager) {}

void AirTouchEventHandler::HandleTouchEvent(TemplateAssembler *tasm,
                                            const std::string &page_name,
                                            const std::string &name, int tag,
                                            float x, float y, float client_x,
                                            float client_y, float page_x,
                                            float page_y) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirTouchEventHandler::HandleTouchEvent");

  if (tasm == nullptr) {
    LOGE("HandleTouchEvent error: tasm is null.");
    return;
  }

  EventOption option = {.bubbles_ = true,
                        .composed_ = true,
                        .capture_phase_ = true,
                        .lepus_event_ = false,
                        .from_frontend_ = false};

  const auto &events = GetEventOperation(
      name, GenerateResponseChain(tag, option), option, long_press_consumed_);
  for (const auto &op : events) {
    FireTouchEvent(tasm, page_name, op.handler, op.target, op.current_target, x,
                   y, client_x, client_y, page_x, page_y);
  }
}

void AirTouchEventHandler::FireTouchEvent(
    TemplateAssembler *tasm, const std::string &page_name,
    const EventHandler *handler, const AirElement *target,
    const AirElement *current_target, float x, float y, float client_x,
    float client_y, float page_x, float page_y) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirTouchEventHandler::FireTouchEvent");

  const lepus::Value &value =
      GetTouchEventParam(handler->name(), target, current_target, x, y,
                         client_x, client_y, page_x, page_y);

  bool in_component = current_target->InComponent();
  if (!in_component) {
    SendPageEvent(tasm, EventType::kTouch, page_name, handler->name().str(),
                  handler->function().str(), value, target);
  } else {
    SendComponentEvent(tasm, EventType::kTouch,
                       current_target->GetParentComponent()->impl_id(),
                       handler->name().str(), handler->function().str(), value,
                       target);
  }
}

size_t AirTouchEventHandler::TriggerComponentEvent(
    TemplateAssembler *tasm, const std::string &event_name,
    const lepus::Value &data) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "AirTouchEventHandler::TriggerComponentEvent");

  BASE_STATIC_STRING_DECL(kComponentId, "componentId");
  lepus_value component_id = data.GetProperty(kComponentId);
  auto id =
      static_cast<int>(component_id.IsInt64() ? component_id.Int64() : -1);
  if (id < 0 || !data.IsObject()) {
    LOGE("TriggerComponentEvent error: component id or data is null.");
    return 0;
  }

  BASE_STATIC_STRING_DECL(kDetail, "detail");
  BASE_STATIC_STRING_DECL(kEventDetail, "eventDetail");
  BASE_STATIC_STRING_DECL(kEventOption, "eventOption");
  bool bubbles = false;
  bool composed = false;
  bool capture_phase = false;
  lepus_value eventOption = data.GetProperty(kEventOption);
  if (eventOption.IsObject()) {
    BASE_STATIC_STRING_DECL(kBubbles, "bubbles");
    BASE_STATIC_STRING_DECL(kComposed, "composed");
    BASE_STATIC_STRING_DECL(kCapturePhase, "capturePhase");
    bubbles = eventOption.GetProperty(kBubbles).IsTrue();
    composed = eventOption.GetProperty(kComposed).IsTrue();
    capture_phase = eventOption.GetProperty(kCapturePhase).IsTrue();
  }

  EventOption option{bubbles, composed, capture_phase, .lepus_event_ = false,
                     .from_frontend_ = true};
  const auto &chain = GenerateResponseChain(id, option, true);
  std::vector<AirEventOperation> ops{};
  GenerateEventOperation(chain, event_name, option, ops);
  if (tasm) {  // for unittest, tasm will be nullptr;
    HandleEventOperation(tasm, event_name, data.GetProperty(kEventDetail),
                         kDetail.str(), option, ops);
  }
  return ops.size();
}

std::vector<AirElement *> AirTouchEventHandler::GenerateResponseChain(
    int tag, const EventOption &option, bool componentEvent) {
  std::vector<AirElement *> chain{};
  AirElement *target_node = air_node_manager_->Get(tag).get();

  if (target_node == nullptr) {
    return chain;
  }

  if (option.bubbles_ || option.composed_ || componentEvent) {
    auto root = componentEvent && !option.composed_
                    ? target_node->GetParentComponent()
                    : nullptr;
    while (target_node != nullptr && target_node != root) {
      chain.push_back(target_node);
      target_node = static_cast<AirElement *>(target_node->air_parent());
    }
  } else {
    chain.push_back(target_node);
  }
  return chain;
}

lepus::Value AirTouchEventHandler::GetTargetInfo(const AirElement *target) {
  auto dict = lepus::Dictionary::Create();

  auto data_set = lepus::Dictionary::Create();
  for (auto iter = target->data_model().begin();
       iter != target->data_model().end(); ++iter) {
    data_set.get()->SetValue(iter->first, iter->second);
  }

  BASE_STATIC_STRING_DECL(kDataset, "dataset");
  BASE_STATIC_STRING_DECL(kUid, "uid");
  dict.get()->SetValue(kDataset, std::move(data_set));
  dict.get()->SetValue(kUid, target->impl_id());
  return lepus::Value(std::move(dict));
}

lepus::Value AirTouchEventHandler::GetCustomEventParam(
    const std::string &name, const std::string &pname,
    const EventOption &option, AirElement *target, AirElement *current_target,
    const lepus::Value &data) const {
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  BASE_STATIC_STRING_DECL(kTarget, "target");

  auto dict = lepus::Dictionary::Create();
  dict.get()->SetValue(kType, name);
  int64_t cur = lynx::base::CurrentSystemTimeMilliseconds();
  dict.get()->SetValue(kTimestamp, static_cast<int64_t>(cur));
  dict.get()->SetValue(kCurrentTarget, GetTargetInfo(current_target));
  dict.get()->SetValue(kTarget, GetTargetInfo(target));
  dict.get()->SetValue(base::String(pname), data);
  return lepus::Value(std::move(dict));
}

void AirTouchEventHandler::HandleCustomEvent(TemplateAssembler *tasm,
                                             const std::string &name, int tag,
                                             const lepus::Value &params,
                                             const std::string &pname) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirTouchEventHandler::HandleCustomEvent");
  LOGI("SendCustomEvent event name:" << name << " tag:" << tag);

  if (tasm == nullptr) {
    LOGE("HandleCustomEvent error: tasm or page is null.");
    return;
  }

  EventOption option;
  std::vector<AirEventOperation> ops{};
  GenerateEventOperation(GenerateResponseChain(tag, option), name, option, ops);
  HandleEventOperation(tasm, name, params, pname, option, ops);
}

void AirTouchEventHandler::HandleEventOperation(
    TemplateAssembler *tasm, const std::string &name,
    const lepus::Value &params, const std::string &pname,
    const EventOption &option, const std::vector<AirEventOperation> &ops) {
  for (const auto &op : ops) {
    if (!op.target->InComponent()) {
      SendPageEvent(tasm, EventType::kCustom, "", name,
                    op.handler->function().str(),
                    GetCustomEventParam(name, pname, option, op.target,
                                        op.current_target, params),
                    op.current_target);
    } else {
      SendComponentEvent(tasm, EventType::kCustom,
                         op.current_target->GetParentComponent()->impl_id(),
                         name, op.handler->function().str(),
                         GetCustomEventParam(name, pname, option, op.target,
                                             op.current_target, params),
                         op.current_target);
    }
  }
}

lepus::Value AirTouchEventHandler::GetTouchEventParam(
    const base::String &handler, const AirElement *target,
    const AirElement *current_target, float x, float y, float client_x,
    float client_y, float page_x, float page_y) const {
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kTarget, "target");
  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  BASE_STATIC_STRING_DECL(kDetail, "detail");
  BASE_STATIC_STRING_DECL(kPageX, "pageX");
  BASE_STATIC_STRING_DECL(kPageY, "pageY");
  BASE_STATIC_STRING_DECL(kClientX, "clientX");
  BASE_STATIC_STRING_DECL(kClientY, "clientY");
  BASE_STATIC_STRING_DECL(kIdentifier, "identifier");
  BASE_STATIC_STRING_DECL(kTouches, "touches");
  BASE_STATIC_STRING_DECL(kChangedTouches, "changedTouches");

  float layouts_unit_per_px =
      current_target->element_manager()->GetLynxEnvConfig().LayoutsUnitPerPx();
  auto dict = lepus::Dictionary::Create();
  dict.get()->SetValue(kType, handler);
  long long cur = lynx::base::CurrentSystemTimeMilliseconds();
  dict.get()->SetValue(kTimestamp, static_cast<int64_t>(cur));

  dict.get()->SetValue(kTarget, GetTargetInfo(target));
  dict.get()->SetValue(kCurrentTarget, GetTargetInfo(current_target));

  auto detail = lepus::Dictionary::Create();
  detail.get()->SetValue(kX, client_x / layouts_unit_per_px);
  detail.get()->SetValue(kY, client_y / layouts_unit_per_px);

  dict.get()->SetValue(kDetail, std::move(detail));

  auto touch = lepus::Dictionary::Create();
  touch.get()->SetValue(kPageX, page_x / layouts_unit_per_px);
  touch.get()->SetValue(kPageY, page_y / layouts_unit_per_px);
  touch.get()->SetValue(kClientX, client_x / layouts_unit_per_px);
  touch.get()->SetValue(kClientY, client_y / layouts_unit_per_px);

  touch.get()->SetValue(kX, x / layouts_unit_per_px);
  touch.get()->SetValue(kY, y / layouts_unit_per_px);
  int64_t identifier = reinterpret_cast<int64_t>(&touch);
  touch.get()->SetValue(kIdentifier, identifier);

  auto touch_value = lepus_value(std::move(touch));

  auto touches = lepus::CArray::Create();
  touches.get()->emplace_back(touch_value);

  dict.get()->SetValue(kTouches, std::move(touches));

  auto changed_touches = lepus::CArray::Create();
  changed_touches.get()->emplace_back(std::move(touch_value));
  dict.get()->SetValue(kChangedTouches, std::move(changed_touches));

  return lepus::Value(std::move(dict));
}

bool AirTouchEventHandler::GenerateEventOperation(
    const std::vector<AirElement *> &response_chain,
    const std::string &event_name, const EventOption &option,
    std::vector<AirEventOperation> &operation) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "AirTouchEventHandler::GenerateEventOperation");
  if (response_chain.empty()) {
    LOGE(
        "Lynx_air HandleEventInternal failed, response_chain empty & "
        "event_name "
        "is" +
        event_name);
    return false;
  }

  AirElement *target = *response_chain.begin();

  bool consume = false;
  bool capture = false;
  // candidate AirElement for handling current event
  AirElement *cur_target = nullptr;
  // handle capture_phase event, namely capture-catch & capture-bind
  if (option.capture_phase_) {
    for (auto iter = response_chain.rbegin(); iter != response_chain.rend();
         ++iter) {
      cur_target = *iter;
      if (cur_target == nullptr) {
        break;
      }
      EventHandler *handler = GetEventHandler(cur_target, event_name);
      if (!handler) {
        continue;
      }
      if (handler->IsCaptureCatchEvent()) {
        operation.push_back({handler, target, cur_target, false});
        capture = true;
        consume = true;
        break;
      } else if (handler->IsCaptureBindEvent()) {
        operation.push_back({handler, target, cur_target, false});
        consume = true;
      }
    }
  }

  // if event is not yet captured, then handle bindEvent & catchEvent
  if (!capture) {
    for (auto iter = response_chain.begin(); iter != response_chain.end();
         ++iter) {
      cur_target = *iter;
      if (cur_target == nullptr) {
        break;
      }
      EventHandler *handler = GetEventHandler(cur_target, event_name);
      if (!handler) {
        continue;
      }
      if (handler->IsCatchEvent()) {
        operation.push_back({handler, target, cur_target, false});
        consume = true;
        break;
      } else if (handler->IsBindEvent()) {
        operation.push_back({handler, target, cur_target, false});
        consume = true;
        if (!option.bubbles_) {
          break;
        }
      }
    }  // for
  }    // if
  return consume;
}

std::string AirTouchEventHandler::GetEventType(const EventType &type) const {
  std::string str;
  switch (type) {
    case EventType::kTouch:
      str = "TouchEvent";
      break;
    case EventType::kCustom:
      str = "CustomEvent";
      break;
    case EventType::kComponent:
      str = "ComponentEvent";
      break;
    case EventType::kBubble:
      str = "BubbleEvent";
      break;
    default:
      str = "UnknownEvent";
      break;
  }
  return str;
}

/// send event to lepus
void AirTouchEventHandler::SendPageEvent(TemplateAssembler *tasm,
                                         const std::string &handler,
                                         const lepus::Value &info) const {
  SendPageEvent(tasm, EventType::kCustom, "card", handler, handler, info,
                nullptr);
}

// use template-api as callback
// e.g.: 'path.function_name'
void AirTouchEventHandler::SendBaseEvent(TemplateAssembler *tasm,
                                         const std::string &event_name,
                                         const std::string &handler,
                                         const lepus::Value &info,
                                         const AirElement *target) const {
  auto context = tasm->FindEntry(DEFAULT_ENTRY_NAME)->GetVm();

  if (context) {
    auto dot_pos = handler.find('.');
    lepus::Value p1(handler.substr(0, dot_pos));
    lepus::Value p2(handler.substr(dot_pos + 1, handler.size() - dot_pos - 1));
    lepus::Value p3 = info.IsTrue() ? info : lepus::Value();
    lepus::Value p4(GetComponentTarget(tasm, target)->impl_id());

    // use template-api to handle event
    BASE_STATIC_STRING_DECL(kCallBaseEvent, "$callBaseEvent");
    context->Call(kCallBaseEvent, p1, p2, p3, p4);
  }
}

void AirTouchEventHandler::SendPageEvent(TemplateAssembler *tasm,
                                         const EventType &type,
                                         const std::string &page_name,
                                         const std::string &event_name,
                                         const std::string &handler,
                                         const lepus::Value &info,
                                         const AirElement *target) const {
  auto context = tasm->FindEntry(DEFAULT_ENTRY_NAME)->GetVm();

  LOGI("lynx_air, SendPageEvent, event_name=" << event_name
                                              << ", handler=" << handler);
  if (context) {
    // Two kinds of handler functions for air.
    // 1.template-api function, (name pattern would be like
    // 'path.function_name', e.g.:click_event.handleClickEvent). In this case,
    // split by '.' to get path and function name, store params in vector.
    // 2. lepus function, (e.g.: handleClickEvent)
    if (handler.find('.') != std::string::npos) {
      SendBaseEvent(tasm, event_name, handler, info, target);
      return;
    }
    auto *element = GetComponentTarget(tasm, target);
    if (element) {
      lepus::Value p1(handler);
      lepus::Value p2 = info.IsTrue() ? info : lepus::Value();
      lepus::Value p3(element->impl_id());
      // use template-api to handle event
      BASE_STATIC_STRING_DECL(kCallPageEvent, "$callPageEvent");
      context->Call(kCallPageEvent, p1, p2, p3);
    }
  }
}

void AirTouchEventHandler::SendComponentEvent(
    TemplateAssembler *tasm, const EventType &type, const int component_id,
    const std::string &event_name, const std::string &handler,
    const lepus::Value &info, const AirElement *target) const {
  auto context = tasm->FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
  if (context) {
    if (handler.find('.') != std::string::npos) {
      SendBaseEvent(tasm, event_name, handler, info, target);
      return;
    }

    lepus::Value p1(static_cast<int32_t>(component_id));
    lepus::Value p2(handler);
    lepus::Value p4(GetComponentTarget(tasm, target, false)->impl_id());
    BASE_STATIC_STRING_DECL(kCallComponentEvent, "$callComponentEvent");
    context->Call(kCallComponentEvent, p1, p2, info, p4);
  }
}

void AirTouchEventHandler::SendComponentEvent(TemplateAssembler *tasm,
                                              const std::string &event_name,
                                              const int component_id,
                                              const lepus::Value &params,
                                              const std::string &param_name) {
  auto shared_component = air_node_manager_->Get(component_id);
  if (!shared_component) {
    return;
  }

  SendComponentEvent(tasm, EventType::kCustom, component_id, event_name,
                     event_name, lepus::Value(), shared_component.get());
}

EventHandler *AirTouchEventHandler::GetEventHandler(
    AirElement *cur_target, const std::string &event_name) {
  auto &map = cur_target->event_map();
  auto event_handler = map.find(event_name);
  if (event_handler == map.end()) {
    return nullptr;
  }
  return (*event_handler).second.get();
}

std::vector<AirEventOperation> AirTouchEventHandler::GetEventOperation(
    const std::string &event_name, const std::vector<AirElement *> &chain,
    const EventOption &option, bool &long_press_consumed) {
  if (event_name == EVENT_TOUCH_START) {
    long_press_consumed = false;
  }
  std::vector<AirEventOperation> ops{};
  if (long_press_consumed && event_name == EVENT_TAP) {
    LOGE("Lynx_air, Send Tap Event failed, long press consumed");
    return ops;
  }

  const auto &consume = GenerateEventOperation(chain, event_name, option, ops);
  if (event_name == EVENT_LONG_PRESS) {
    long_press_consumed = consume;
  }
  return ops;
}

// Get target related component AirElement. If target is component and
// ignore_target is false, return itself; else return its parent component. If
// target is nullptr, return page element.
const AirElement *AirTouchEventHandler::GetComponentTarget(
    TemplateAssembler *tasm, const AirElement *target,
    bool ignore_target) const {
  if (target) {
    if (!ignore_target && target->is_component()) {
      return target;
    } else {
      return target->GetParentComponent();
    }
  } else {
    return static_cast<AirElement *>(
        tasm->page_proxy()->element_manager()->AirRoot());
  }
}

}  // namespace tasm
}  // namespace lynx

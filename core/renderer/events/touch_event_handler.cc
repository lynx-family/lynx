// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/events/touch_event_handler.h"

#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/replay/replay_controller.h"
#include "third_party/rapidjson/document.h"

#if ENABLE_LEPUSNG_WORKLET
#include "core/renderer/worklet/lepus_component.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_raf_handler.h"
#include "core/runtime/bindings/napi/worklet/napi_func_callback.h"
#endif  // ENABLE_LEPUSNG_WORKLET

namespace lynx {
namespace tasm {

#define EVENT_TOUCH_START "touchstart"
#define EVENT_TOUCH_MOVE "touchmove"
#define EVENT_TOUCH_CANCEL "touchcancel"
#define EVENT_TOUCH_END "touchend"
#define EVENT_TAP "tap"
#define EVENT_LONG_PRESS "longpress"

constexpr const static char *kDetail = "detail";

static void AddTimestampProperty(lepus::Dictionary *params,
                                 int64_t timestamp = 0) {
  if (params) {
    BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
    timestamp = timestamp != 0
                    ? timestamp
                    : std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
    params->SetValue(kTimestamp, timestamp);
  }
}

TouchEventHandler::TouchEventHandler(
    NodeManager *node_manager,
    runtime::ContextProxy::Delegate &context_proxy_delegate,
    bool support_component_js, bool use_lepus_ng, const std::string &version)
    : node_manager_(node_manager),
      context_proxy_delegate_(context_proxy_delegate),
      support_component_js_(support_component_js),
      use_lepus_ng_(use_lepus_ng),
      version_(version),
      current_touches_(lepus_value(lepus::CArray::Create())) {
#if ENABLE_LEPUSNG_WORKLET
  task_handler_ = std::make_shared<worklet::LepusApiHandler>();
#endif  // ENABLE_LEPUSNG_WORKLET
  LOGI("TouchEventHandler init: support_component_js_: "
       << (support_component_js_ ? "true" : "false")
       << "; use_lepus_ng_: " << (use_lepus_ng_ ? "true" : "false"));
  if (!support_component_js_) {
    // Report the situation where support_component_js_=false. If no online
    // templates rely on this behavior, this flag will be removed in the future.
    report::FeatureCounter::Instance()->Count(
        report::LynxFeature::CPP_DISABLE_SUPPORT_COMPONENT_JS);
  }
}

void TouchEventHandler::HandleEventOperations(TemplateAssembler *tasm,
                                              EventContext &context,
                                              const EventOpsVector &ops) {
  bool stop_immediate_propagation = false;
  const EventOperation *stop_propagation_op = nullptr;
  for (const auto &op : ops) {
    bool is_js_event = true;
    // js global event handler is nullptr
    if (op.handler_) {
      is_js_event = op.handler_->is_js_event();
    }
    const lepus_value &params =
        context.get_event_params(op.target_, op.current_target_, is_js_event);
    if (op.global_event_) {
      SendGlobalEvent(context.event_type, context.event_name, params);
    } else {
      // trigger jsb event
      if (op.handler_->is_piper_event()) {
        TriggerLepusBridgesAsync(context.event_type, tasm, context.event_name,
                                 *(op.handler_->piper_event_vec()));
        continue;
      }

      if (stop_immediate_propagation ||
          (stop_propagation_op &&
           !stop_propagation_op->IsSameTargetAndEventPhase(op))) {
        continue;
      }

      if (!op.handler_->is_js_event() && use_lepus_ng_) {
        EventResult result = FireElementWorklet(
            context, op.current_target_->ParentComponentIdString(),
            op.current_target_->ParentComponentEntryName(), tasm, op.handler_,
            params, op.current_target_->impl_id());
        if (result == EventResult::kStopImmediatePropagation) {
          // If stopImmediatePropagation() is invoked during one such call, no
          // remaining listeners will be called, either on that element or any
          // other element.
          stop_immediate_propagation = true;
        } else if (result == EventResult::kStopPropagation) {
          // stopPropagation() prevents further propagation of the current event
          // in the capturing and bubbling phases.
          stop_propagation_op = &op;
        }
        continue;
      }

      if (tasm->page_proxy()->element_manager()->IsAirModeFiberEnabled()) {
        FireEventForAir(tasm, context.event_type, context.page_name,
                        op.handler_, op.target_, op.current_target_, params);
      } else {
        FireEvent(context.event_type, context.page_name, op.handler_,
                  op.target_, op.current_target_, params);
      }
    }
  }
}

void TouchEventHandler::HandleTouchEvent(TemplateAssembler *tasm,
                                         const std::string &page_name,
                                         const std::string &name,
                                         const EventInfo &info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TouchEventHandler::HandleTouchEvent",
              "page_name", page_name, "name", name);
  LOGI("HandleTouchEvent page:" << page_name << " ;event: " << name
                                << " tag:" << info.tag
                                << " ;multiFinger:" << info.is_multi_finger);
  if (tasm == nullptr || tasm->page_proxy() == nullptr) {
    LOGE("HandleTouchEvent error: tasm or page is null.");
    return;
  }

  const auto &f = [this, name](const auto &chain, const auto &option,
                               bool &long_press_consumed) {
    if (name == EVENT_TOUCH_START) {
      // TODO(hexionghui): Fix the problem: When one finger is long pressed and
      // one finger is tapped, the tap event will be triggered after the long
      // press is released.
      long_press_consumed = false;
    }
    EventOpsVector ops;
    // TODO(hexionghui): Unify the logic of tap and click: When the long press
    // event is not bound, the tap and click events are triggered after the long
    // press, otherwise do not trigger the tap end click events.
    if (long_press_consumed && name == EVENT_TAP) {
      LOGI("Lynx Send Tap Event failed, longpress consumed");
      return ops;
    }
    const auto &consume = HandleEventInternal(chain, name, option, ops);
    if (name == EVENT_LONG_PRESS) {
      long_press_consumed = consume;
    }
    return ops;
  };

  EventContext context = {
      .event_type = EventType::kTouch,
      .event_name = name,
      .page_name = page_name,
      .option = {.bubbles_ = true,
                 .composed_ = true,
                 .capture_phase_ = true,
                 .lepus_event_ = false,
                 .from_frontend_ = false},
      .get_event_params = [this, &name, &info](Element *target,
                                               Element *current_target,
                                               bool is_js_event) {
        return GetTouchEventParam(name, target, current_target, info,
                                  is_js_event);
      }};

  if (info.is_multi_finger) {
    for (auto events : *info.params.Table()) {
      int tag = std::stoi(events.first.str());
      const auto &chain = GenerateResponseChain(tag, context.option);
      auto ops = f(chain, context.option, long_press_consumed_);
      HandleEventOperations(tasm, context, ops);
    }
    if (name == EVENT_TOUCH_CANCEL) {
      current_touches_ = lepus_value(lepus::CArray::Create());
    }
  } else {
    const auto &chain = GenerateResponseChain(info.tag, context.option);
    auto ops = f(chain, context.option, long_press_consumed_);
    HandleEventOperations(tasm, context, ops);
  }

  return;
}

/**

  Handle a gesture event, using worklet
* @param tasm A pointer to the TemplateAssembler object
* @param name The name of the gesture event
* @param tag The unique identifier for the target element
* @param gesture_id The id for the gesture event
* @param param The Lepus script / object associated with the gesture event
*/
void TouchEventHandler::HandleGestureEvent(TemplateAssembler *tasm,
                                           const base::String &name, int tag,
                                           int gesture_id,
                                           const lepus::Value &params) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TouchEventHandler::HandleGestureEvent",
              "name", name.str());
  // Check if using LepusNG
  if (!use_lepus_ng_) {
    LOGE("HandleGestureEvent error: not use lepus ng.");
    return;
  }

  // Check if tasm and page is not null
  if (!tasm || !tasm->page_proxy()) {
    LOGE("HandleGestureEvent error: tasm or page is null.");
    return;
  }

  const auto &client = tasm->page_proxy()->element_manager();

  // Ensure that the element manager and node manager are not null.
  if (!client || !client->node_manager()) {
    LOGE("Element manager is null");
    return;
  }

  // Get the target element
  Element *target_node = node_manager_->Get(tag);
  if (!target_node) {
    LOGE("HandleGestureEvent error: target_node is null.");
    return;
  }

  // Retrieve the gesture detector for the specified gesture ID.
  const auto &gesture_map = target_node->gesture_map();
  const auto &gesture_detector = gesture_map.find(gesture_id);
  if (gesture_detector == gesture_map.end()) {
    LOGE("Gesture detector not found for id" << gesture_id);
    return;
  }

  // Retrieve the list of gesture callbacks for the gesture detector.
  const auto &gesture_callbacks = gesture_detector->second->gesture_callbacks();
  if (gesture_callbacks.empty()) {
    LOGE("No gesture callbacks defined for gesture detector with id "
         << gesture_id);
    return;
  }

  // Find the gesture callback with the specified name.
  const auto &it = std::find_if(
      gesture_callbacks.begin(), gesture_callbacks.end(),
      [&name](const auto &callback) { return callback.name_ == name; });
  if (it == gesture_callbacks.end()) {
    LOGE("Gesture callback with name " << name.str() << " not found");
    return;
  }

  std::unique_ptr<EventHandler> handler = nullptr;
  BASE_STATIC_STRING_DECL(kGesture, "Gesture");

  // Create an event handler
  if (tasm->EnableFiberArch()) {
    handler = std::make_unique<EventHandler>(kGesture, name, it->lepus_object_,
                                             it->ctx_);
    EnsureGestureManager(it->ctx_);
  } else {
    handler = std::make_unique<EventHandler>(kGesture, name, it->lepus_script_,
                                             it->lepus_function_);
  }

  EventContext context = {.event_type = EventType::kGesture,
                          .event_name = name.str(),
                          .page_name = "",
                          .option = {.bubbles_ = false,
                                     .composed_ = false,
                                     .capture_phase_ = false,
                                     .lepus_event_ = true,
                                     .from_frontend_ = false}};
  FireElementWorklet(
      context, target_node->ParentComponentIdString(),
      target_node->ParentComponentEntryName(), tasm, handler.get(),
      GetCustomEventParam(name.str(), "params", context.option, target_node,
                          target_node, params, false),
      target_node->impl_id());
}

void TouchEventHandler::HandleCustomEvent(TemplateAssembler *tasm,
                                          const std::string &name, int tag,
                                          const lepus::Value &params,
                                          const std::string &pname) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TouchEventHandler::HandleCustomEvent",
              "name", name);
  LOGI("SendCustomEvent event name:" << name << " tag:" << tag);

  if (tasm == nullptr || tasm->page_proxy() == nullptr) {
    LOGE("HandleCustomEvent error: tasm or page is null.");
    return;
  }

  EventOption option = {.bubbles_ = false,
                        .composed_ = false,
                        .capture_phase_ = false,
                        .lepus_event_ = false,
                        .from_frontend_ = false};
  EventOpsVector ops;
  const auto &chain = GenerateResponseChain(tag, option);
  HandleEventInternal(chain, name, option, ops);
  EventContext context = {
      .event_type = EventType::kCustom,
      .event_name = name,
      .page_name = "",
      .option = option,
      .get_event_params = [this, &name, &pname, &option, &params](
                              Element *target, Element *current_target,
                              bool is_js_event) {
        return GetCustomEventParam(name, pname, option, target, current_target,
                                   params, is_js_event);
      }};
  HandleEventOperations(tasm, context, ops);
  return;
}

void TouchEventHandler::HandlePseudoStatusChanged(int32_t id,
                                                  PseudoState pre_status,
                                                  PseudoState current_status) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "TouchEventHandler::HandlePseudoStatusChanged");
  LOGI("HandlePseudoStatusChanged sign:"
       << id << " , with pre_status: " << pre_status
       << " , and current_status:" << current_status);
  Element *element = node_manager_->Get(id);
  if (element) {
    element->OnPseudoStatusChanged(pre_status, current_status);
  }
}

void TouchEventHandler::FireEvent(const EventType &type,
                                  const std::string &page_name,
                                  const EventHandler *handler,
                                  const Element *target,
                                  const Element *current_target,
                                  const lepus::Value &params) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TouchEventHandler::FireEvent");

  bool in_component = current_target->InComponent();
  if (!support_component_js_ || !in_component) {
    SendPageEvent(type, page_name, handler->name().str(),
                  handler->function().str(), params);
  } else {
    PublishComponentEvent(type, current_target->ParentComponentIdString(),
                          handler->name().str(), handler->function().str(),
                          params);
  }
}

void TouchEventHandler::FireEventForAir(TemplateAssembler *tasm,
                                        const EventType &type,
                                        const std::string &page_name,
                                        const EventHandler *handler,
                                        const Element *target,
                                        const Element *current_target,
                                        const lepus::Value &params) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TouchEventHandler::FireEventForAir");

  // In Air Mode, page/component's event triggered by specific lepus function.
  bool in_component = current_target->InComponent();
  auto vmContext = tasm->FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
  auto *parentComponent = current_target->GetParentComponentElement();
  if (!in_component) {
    LOGI("lynx_air, SendPageEvent, event_name="
         << handler->name().str() << ", handler=" << handler->function().str());
    if (parentComponent) {
      BASE_STATIC_STRING_DECL(kCallPageEvent, "$callPageEvent");
      vmContext->Call(kCallPageEvent, lepus_value(handler->function()), params,
                      lepus::Value(parentComponent->impl_id()));
    }
  } else {
    BASE_STATIC_STRING_DECL(kCallComponentEvent, "$callComponentEvent");
    vmContext->Call(kCallComponentEvent,
                    lepus::Value(parentComponent->impl_id()),
                    lepus_value(handler->function()), params,
                    lepus::Value(target->impl_id()));
  }
}

void TouchEventHandler::ApplyEventTargetParams(lepus::DictionaryPtr params,
                                               const Element *target,
                                               const Element *current_target,
                                               bool is_js_event) const {
  if (!params || !target || !current_target) {
    return;
  }

  AddTimestampProperty(params.get());

  BASE_STATIC_STRING_DECL(kTarget, "target");
  params->SetValue(
      kTarget, GetTargetInfo(target->impl_id(), target->data_model(), target,
                             is_js_event));

  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  params->SetValue(kCurrentTarget, GetTargetInfo(current_target->impl_id(),
                                                 current_target->data_model(),
                                                 current_target, is_js_event));
}

void TouchEventHandler::HandleBubbleEvent(TemplateAssembler *tasm,
                                          const std::string &page_name,
                                          const std::string &name, int tag,
                                          lepus::DictionaryPtr params) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TouchEventHandler::HandleBubbleEvent",
              "page_name", page_name, "name", name);
  LOGI("HandleBubbleEvent page:" << page_name << " ;event: " << name
                                 << " tag:" << tag);
  if (tasm == nullptr || tasm->page_proxy() == nullptr) {
    LOGE("HandleBubbleEvent error: tasm or page is null.");
    return;
  }
  // HandleTouchEvent will handle tap and long press event,
  // use EVENT_TOUCH_START to reset long press state,
  // but no touch event on PC,
  // so need to use the mousedown event to reset long press state.
  if (name.compare("mousedown") == 0) {
    long_press_consumed_ = false;
  }

  EventContext context = {
      .event_type = EventType::kBubble,
      .event_name = name,
      .page_name = page_name,
      .option = {.bubbles_ = true,
                 .composed_ = true,
                 .capture_phase_ = true,
                 .lepus_event_ = false,
                 .from_frontend_ = false},
      .get_event_params = [this, &params](Element *target,
                                          Element *current_target,
                                          bool is_js_event) {
        ApplyEventTargetParams(params, target, current_target, is_js_event);
        return lepus::Value::Clone(lepus::Value(params));
      }};
  const auto &chain = GenerateResponseChain(tag, context.option);
  EventOpsVector ops;
  HandleEventInternal(chain, name, context.option, ops);
  HandleEventOperations(tasm, context, ops);
}

void TouchEventHandler::CallJSFunctionInLepusEvent(
    const std::string &component_id, const std::string &name,
    const lepus::Value &params) {
#if ENABLE_LEPUSNG_WORKLET
  auto args = lepus::CArray::Create();
  args->emplace_back(component_id);
  args->emplace_back(name);
  // info be ShallowCopy first to avoid to be marked const.
  args->emplace_back(lepus_value::ShallowCopy(params));
  runtime::MessageEvent event(
      runtime::kMessageEventTypeCallJSFunctionInLepusEvent,
      runtime::ContextProxy::Type::kCoreContext,
      runtime::ContextProxy::Type::kJSContext, lepus::Value(std::move(args)));
  context_proxy_delegate_.DispatchMessageEvent(std::move(event));
#endif
}

void TouchEventHandler::HandleTriggerComponentEvent(
    TemplateAssembler *tasm, const std::string &event_name,
    const lepus::Value &data) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "TouchEventHandler::HandleTriggerComponentEvent", "name",
              event_name);
  LOGI("HandleTriggerComponentEvent event: " << event_name);
  if (tasm == nullptr || tasm->page_proxy() == nullptr) {
    LOGE("TriggerComponentEvent error: page is null.");
    return;
  }
  auto page = tasm->page_proxy();
  if (!data.IsObject()) {
    LOGE("TriggerComponentEvent error: data is not table.");
    return;
  }

  BASE_STATIC_STRING_DECL(kEventDetail, "eventDetail");
  BASE_STATIC_STRING_DECL(kComponentId, "componentId");
  lepus_value msg = data.GetProperty(kEventDetail);
  lepus_value component_id = data.GetProperty(kComponentId);
  std::string id = component_id.IsString() ? component_id.StdString() : "";
  if (id.empty()) {
    LOGE("TriggerComponentEvent error: not set component id.");
    return;
  }

  bool bubbles = false;
  bool composed = false;
  bool capture_phase = false;
  BASE_STATIC_STRING_DECL(kEventOption, "eventOption");
  if (data.Contains(kEventOption)) {
    lepus_value ops = data.GetProperty(kEventOption);
    if (ops.IsObject()) {
      BASE_STATIC_STRING_DECL(kBubbles, "bubbles");
      if (auto v = ops.GetProperty(kBubbles); v.IsBool()) {
        bubbles = v.Bool();
      }
      BASE_STATIC_STRING_DECL(kComposed, "composed");
      if (auto v = ops.GetProperty(kComposed); v.IsBool()) {
        composed = v.Bool();
      }
      BASE_STATIC_STRING_DECL(kCapturePhase, "capturePhase");
      if (auto v = ops.GetProperty(kCapturePhase); v.IsBool()) {
        capture_phase = v.Bool();
      }
    }
  }

  Element *component_element = nullptr;
  // In radon diff, execute HandleTriggerComponent, use Radon Node to find
  // Element. This is because in the list, there are currently some situations
  // where the component element corresponding to the Component ID is a wild
  // pointer, and the root cause has not yet been identified, so it is changed
  // to use Radon Node to find the Element. This avoids the occasional crash
  // online. After the root cause is found, change back to the original logic.
  // And NoDiff maintains the original logic.
  if (tasm->EnableFiberArch()) {
    component_element = page->ComponentElementWithStrId(id);
  } else {
    int64_t long_id;
    constexpr const static int kBase = 10;
    if (!base::StringToInt(id, long_id, kBase)) {
      return;
    }
    RadonComponent *component =
        page->ComponentWithId(static_cast<int>(long_id));
    // Enabling devtool will re-add elements to the component where the element
    // has been removed, resulting in inconsistent performance between opening
    // devtool and closing devtool when removeComponentElement is on. Therefore,
    // we add NeedsElement judgment to align.
    if (component == nullptr || component->element() == nullptr ||
        !component->NeedsElement()) {
      LOGE(
          "TriggerComponentEvent error: can not find the specific component or "
          "the element of the radon component is empty.");
      return;
    }
    component_element = component->element();
  }

  if (component_element == nullptr) {
    LOGE("TriggerComponentEvent error: can not find component.");
    return;
  }

  EventOption option{bubbles, composed, capture_phase, .lepus_event_ = false,
                     .from_frontend_ = true};
  EventOpsVector ops;
  const auto &chain = GenerateResponseChain(page, component_element, option);
  HandleEventInternal(chain, event_name, option, ops);
  EventContext context = {
      .event_type = EventType::kComponent,
      .event_name = event_name,
      .page_name = "",
      .option = option,
      .get_event_params = [this, &event_name, &option, &msg](
                              Element *target, Element *current_target,
                              bool is_js_event) {
        return GetCustomEventParam(event_name, kDetail, option, target,
                                   current_target, msg, is_js_event);
      }};
  HandleEventOperations(tasm, context, ops);
  return;
}

void TouchEventHandler::HandleJSCallbackLepusEvent(const int64_t callback_id,
                                                   TemplateAssembler *tasm,
                                                   const lepus::Value &data) {
#if ENABLE_LEPUSNG_WORKLET
  task_handler_->InvokeWithTaskID(callback_id, data, tasm);
#endif
}

TouchEventHandler::ResponseChainVector TouchEventHandler::GenerateResponseChain(
    int tag, const EventOption &option) {
  // Should always return variable chain to make NRVO work.
  ResponseChainVector chain;
  Element *target_node = node_manager_->Get(tag);

  if (target_node == nullptr) {
    return chain;
  }

  // If the fiber element is currently in the detached state, then do not
  // generate the corresponding chain.
  if (target_node->is_fiber_element() &&
      static_cast<FiberElement *>(target_node)->IsDetached()) {
    LOGE(
        "TouchEventHandler::GenerateResponseChain failed since the target node "
        << target_node->GetTag().str()
        << " with sign: " << target_node->impl_id() << " is detached.");
    return chain;
  }

  if (option.bubbles_) {
    while (target_node != nullptr) {
      chain.push_back(target_node);
      target_node = static_cast<Element *>(target_node->parent());
    }
  } else {
    chain.push_back(target_node);
  }
  return chain;
}

TouchEventHandler::ResponseChainVector TouchEventHandler::GenerateResponseChain(
    PageProxy *proxy, Element *component, const EventOption &option) {
  // Should always return variable chain to make NRVO work.
  ResponseChainVector chain;
  if (component == nullptr) {
    return chain;
  }

  // If the fiber element is currently in the detached state, then do not
  // generate the corresponding chain.
  if (component->is_fiber_element() &&
      static_cast<FiberElement *>(component)->IsDetached()) {
    LOGE(
        "TouchEventHandler::GenerateResponseChain failed since the component "
        "with sign: "
        << component->impl_id() << " is detached.");
    return chain;
  }

  chain.push_back(component);

  auto *root_component = component->GetParentComponentElement();
  auto *current_node = component;

  while (current_node != nullptr) {
    auto *next_node = current_node->parent();
    if (!next_node || current_node == next_node) {
      break;
    }

    current_node = next_node;

    if (current_node == root_component && !option.composed_) {
      break;
    }

    if (current_node->GetParentComponentElement() != root_component &&
        !option.composed_) {
      continue;
    }

    chain.push_back(current_node);
  }

  return chain;
}

lepus::Value TouchEventHandler::GetTouchEventParam(
    const base::String &handler, const Element *target,
    const Element *current_target, const EventInfo &info,
    bool is_js_event) const {
  if (info.is_multi_finger) {
    return GetTouchEventParam(handler, target, current_target, info.params,
                              is_js_event, info.timestamp);
  } else {
    return GetTouchEventParam(handler, target, current_target, info.x, info.y,
                              info.client_x, info.client_y, info.page_x,
                              info.page_y, is_js_event, info.timestamp);
  }
}

lepus::Value TouchEventHandler::GetTouchEventParam(
    const base::String &handler, const Element *target,
    const Element *current_target, float x, float y, float client_x,
    float client_y, float page_x, float page_y, bool is_js_event,
    int64_t timestamp) const {
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kTarget, "target");
  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kDetail, "detail");
  BASE_STATIC_STRING_DECL(kPageX, "pageX");
  BASE_STATIC_STRING_DECL(kPageY, "pageY");
  BASE_STATIC_STRING_DECL(kClientX, "clientX");
  BASE_STATIC_STRING_DECL(kClientY, "clientY");
  BASE_STATIC_STRING_DECL(kIdentifier, "identifier");
  BASE_STATIC_STRING_DECL(kTouches, "touches");
  BASE_STATIC_STRING_DECL(kChangedTouches, "changedTouches");

  auto dict = lepus::Dictionary::Create();
  dict.get()->SetValue(kType, handler);
  AddTimestampProperty(dict.get(), timestamp);
  dict.get()->SetValue(
      kTarget, GetTargetInfo(target->impl_id(), target->data_model(), target,
                             is_js_event));
  dict.get()->SetValue(
      kCurrentTarget,
      GetTargetInfo(current_target->impl_id(), current_target->data_model(),
                    current_target, is_js_event));

  const float layouts_unit_per_px =
      current_target->element_manager()->GetLynxEnvConfig().LayoutsUnitPerPx();
  auto detail = lepus::Dictionary::Create();
  detail.get()->SetValue(kX, page_x / layouts_unit_per_px);
  detail.get()->SetValue(kY, page_y / layouts_unit_per_px);

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
  touches.get()->push_back(touch_value);

  dict.get()->SetValue(kTouches, std::move(touches));

  auto changed_touches = lepus::CArray::Create();
  changed_touches.get()->emplace_back(std::move(touch_value));
  dict.get()->SetValue(kChangedTouches, std::move(changed_touches));

  return lepus::Value(std::move(dict));
}

/**
 * get Touch Event Param for multi touch
 * @param params touch events detail, struct is:
 * params = {
 *   ui1 tag: [
 *     [
 *       touch identifier,
 *       client_x,
 *       client_y,
 *       page_x,
 *       page_y,
 *       x,
 *       y,
 *     ],
 *     [
 *       // the same struct as above
 *     ],
 *   ],
 *   ui2 tag: ...
 * }
 */
lepus_value TouchEventHandler::GetTouchEventParam(const base::String &handler,
                                                  const Element *target,
                                                  const Element *current_target,
                                                  const lepus_value &params,
                                                  bool is_js_event,
                                                  int64_t timestamp) const {
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kTarget, "target");
  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  BASE_STATIC_STRING_DECL(kTouches, "touches");
  BASE_STATIC_STRING_DECL(kChangedTouches, "changedTouches");

  // TODO(songshourui.null): use static variable to instead dict's key
  auto dict = lepus::Dictionary::Create();
  dict.get()->SetValue(kType, handler);
  AddTimestampProperty(dict.get(), timestamp);
  dict.get()->SetValue(
      kTarget, GetTargetInfo(target->impl_id(), target->data_model(), target,
                             is_js_event));
  dict.get()->SetValue(
      kCurrentTarget,
      GetTargetInfo(current_target->impl_id(), current_target->data_model(),
                    current_target, is_js_event));
  // keep detail reserved, no parameter passed now.
  auto detail = lepus::Dictionary::Create();
  float detail_x, detail_y;
  detail_x = detail_y = __FLT_MAX__;
  // You should no longer use detail because it has been moved.

  // if touch_cancel, current_touches will be cleaned after send_event_function
  // is called, because GetTouchEventParam is called in loop
  if (handler == EVENT_TOUCH_CANCEL) {
    dict.get()->SetValue(kChangedTouches, current_touches_);
    dict.get()->SetValue(kTouches, lepus::CArray::Create());
    return lepus_value(std::move(dict));
  }

  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kDetail, "detail");
  BASE_STATIC_STRING_DECL(kPageX, "pageX");
  BASE_STATIC_STRING_DECL(kPageY, "pageY");
  BASE_STATIC_STRING_DECL(kClientX, "clientX");
  BASE_STATIC_STRING_DECL(kClientY, "clientY");
  BASE_STATIC_STRING_DECL(kIdentifier, "identifier");

  // store touches in params
  auto changed_touches = lepus::CArray::Create();
  const float layouts_unit_per_px =
      current_target->element_manager()->GetLynxEnvConfig().LayoutsUnitPerPx();
  for (const auto &ui_events : *(params.Table())) {
    // ui_events: {ui tag, ui events}
    if (ui_events.second.IsArray()) {
      // events: including all touches that target is the same ui.
      auto events = ui_events.second.Array();
      for (size_t i = 0; i < events->size(); ++i) {
        // get event info
        const auto &event_info = events->get(i).Array();

        int64_t identifier = static_cast<int64_t>(event_info->get(0).Number());
        float client_x = event_info->get(1).Number();
        float client_y = event_info->get(2).Number();
        float page_x = event_info->get(3).Number();
        float page_y = event_info->get(4).Number();
        float x = event_info->get(5).Number();
        float y = event_info->get(6).Number();

        if (detail_x == __FLT_MAX__ && detail_y == __FLT_MAX__) {
          detail_x = page_x / layouts_unit_per_px;
          detail_y = page_y / layouts_unit_per_px;
        }

        auto touch = lepus::Dictionary::Create();
        touch->SetValue(kPageX, page_x / layouts_unit_per_px);
        touch->SetValue(kPageY, page_y / layouts_unit_per_px);
        touch->SetValue(kClientX, client_x / layouts_unit_per_px);
        touch->SetValue(kClientY, client_y / layouts_unit_per_px);
        touch->SetValue(kX, x / layouts_unit_per_px);
        touch->SetValue(kY, y / layouts_unit_per_px);
        touch->SetValue(kIdentifier, identifier);

        auto touch_value = lepus_value(std::move(touch));
        changed_touches->push_back(touch_value);

        // find whether this touch identifier is in current_touches_. If it
        // doesn't ui_in_current_touches, then insert it, or modify or delete it
        bool ui_in_current_touches = false;
        auto touches = current_touches_.Array();
        for (size_t j = 0; j < touches->size(); ++j) {
          if (touches->get(j).Table()->GetValue(kIdentifier).Number() ==
              identifier) {
            ui_in_current_touches = true;
            if (handler == EVENT_TOUCH_END) {
              touches->Erase(static_cast<uint32_t>(j));
              break;
            }
            touches->set(j, touch_value);
            break;
          }
        }
        // current_touches don't include touch, add it into current_touches_
        if (handler == EVENT_TOUCH_START && !ui_in_current_touches) {
          touches->emplace_back(std::move(touch_value));
        }
      }
    }
  }
  detail.get()->SetValue(kX, detail_x);
  detail.get()->SetValue(kY, detail_y);
  dict.get()->SetValue(kDetail, std::move(detail));
  dict.get()->SetValue(kChangedTouches, std::move(changed_touches));
  // if not using clone, in further process, current_touches_ will be turned to
  // readonly.
  dict.get()->SetValue(kTouches, lepus::Value::Clone(current_touches_));
  return lepus_value(std::move(dict));
}

lepus::Value TouchEventHandler::GetTargetInfo(int32_t impl_id,
                                              const AttributeHolder *holder,
                                              const Element *element,
                                              bool is_js_event) {
  auto dict = lepus::Dictionary::Create();
  if (holder != nullptr) {
    BASE_STATIC_STRING_DECL(kId, "id");
    BASE_STATIC_STRING_DECL(kDataset, "dataset");
    BASE_STATIC_STRING_DECL(kUid, "uid");

    dict.get()->SetValue(kId, holder->idSelector());
    auto data_set = lepus::Dictionary::Create();
    for (const auto &[key, value] : holder->dataset()) {
      data_set.get()->SetValue(key, value);
    }
    dict.get()->SetValue(kDataset, std::move(data_set));
    dict.get()->SetValue(kUid, impl_id);
  }

  // element ref needed in fiber element worklet
  if (element != nullptr && !is_js_event && element->is_fiber_element()) {
    BASE_STATIC_STRING_DECL(kElementRefptr, "elementRefptr");
    FiberElement *fiberElement =
        static_cast<FiberElement *>(const_cast<Element *>(element));
    dict.get()->SetValue(kElementRefptr,
                         fml::RefPtr<FiberElement>(fiberElement));
  }

  return lepus::Value(std::move(dict));
}

lepus::Value TouchEventHandler::GetCustomEventParam(
    const std::string &name, const std::string &pname,
    const EventOption &option, Element *target, Element *current_target,
    const lepus::Value &data, bool is_js_event) const {
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kTarget, "target");
  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  BASE_STATIC_STRING_DECL(kId, "id");
  BASE_STATIC_STRING_DECL(kDataset, "dataset");
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  BASE_STATIC_STRING_DECL(kParams, "params");
  base::String pname_str(pname);

  auto dict = lepus::Dictionary::Create();
  lepus::Value para(dict);
  dict.get()->SetValue(kType, name);
  int64_t timestamp = 0;
  if (data.IsTable() && data.Table().get()->Contains(kTimestamp)) {
    timestamp = data.Table().get()->GetValue(kTimestamp).Number();
    data.Table().get()->Erase(kTimestamp);
  }
  AddTimestampProperty(dict.get(), timestamp);
  auto current_target_dict =
      GetTargetInfo(current_target->impl_id(), current_target->data_model(),
                    current_target, is_js_event);
  auto target_dict = GetTargetInfo(target->impl_id(), target->data_model(),
                                   target, is_js_event);
  // CustomEvent should contain type, timestamp, target, currentTarget and
  // detail. In the previous version (<= 2.0), Native CustomEvent contains
  // target.id, target.dataset, target.para. To avoid beak change, when
  // engineVersion <= 2.0, add target.id, target.dataset & target.para to
  // dict.
  if (base::Version(version_) < base::Version(LYNX_VERSION_2_1) &&
      !option.from_frontend_) {
    current_target_dict.Table()->SetValue(pname_str, data);
    target_dict.Table()->SetValue(pname_str, data);
    dict.get()->SetValue(kId, target_dict.Table()->GetValue(kId));
    dict.get()->SetValue(kDataset, target_dict.Table()->GetValue(kDataset));
    report::FeatureCounter::Instance()->Count(
        report::LynxFeature::CPP_UI_CUSTOM_EVENT_PARAMETER_BUG);
  }
  dict.get()->SetValue(kCurrentTarget, current_target_dict);
  dict.get()->SetValue(kTarget, target_dict);
  dict.get()->SetValue(pname_str, data);
  if (pname_str.IsEqual(kParams) && !option.from_frontend_) {
    BASE_STATIC_STRING_DECL(kDetail, "detail");
    dict.get()->SetValue(kDetail, data);
  }
  // CustomEvent should contain type, timestamp, target, currentTarget and
  // detail. In the previous version (<= 1.5), FeCustomEvent is actually
  // FeCustomEvent.detail. To avoid beak change, when engineVersion < 1.6,
  // use data's key/value pair override the CustomEvent.
  if (base::Version(version_) < base::Version(LYNX_VERSION_1_6) &&
      option.from_frontend_) {
    if (data.IsObject()) {
      ForEachLepusValue(
          data, [&dict](const lepus::Value &key, const lepus::Value &value) {
            dict->SetValue(key.String(), value);
          });
    } else {
      para = data;
    }
    report::FeatureCounter::Instance()->Count(
        report::LynxFeature::CPP_FE_CUSTOM_EVENT_PARAMETER_BUG);
  }
  return para;
}

bool TouchEventHandler::HandleEventInternal(
    const ResponseChainVector &response_chain, const std::string &event_name,
    const EventOption &option, EventOpsVector &operation) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "HandleEventInternal", "name", event_name);
  if (response_chain.empty()) {
    LOGI(
        "Lynx HandleEventInternal failed, response_chain empty & event_name "
        "is" +
        event_name);
    return false;
  }

  const auto &find_event_f =
      [](const EventMap &map, const std::string &event_name) -> EventHandler * {
    auto find_iter = map.find(event_name);
    if (find_iter == map.end()) {
      return nullptr;
    }
    return (*find_iter).second.get();
  };

  const auto &get_handler_f =
      [&find_event_f](
          Element *cur_target, const std::string &event_name,
          bool global_bind_event) -> base::InlineVector<EventHandler *, 4> {
    base::InlineVector<EventHandler *, 4> res;
    if (global_bind_event) {
      // Find the event handler in the global bind event map
      auto *global_event_handler =
          find_event_f(cur_target->global_bind_event_map(), event_name);
      if (global_event_handler) {
        res.push_back(global_event_handler);
      }
    } else {
      // Find the event handler in the event map
      auto *js_event_handler =
          find_event_f(cur_target->event_map(), event_name);
      if (js_event_handler) {
        res.push_back(js_event_handler);
      }
      // Find the event handler in the lepus event map
      auto *lepus_event_handler =
          find_event_f(cur_target->lepus_event_map(), event_name);
      if (lepus_event_handler) {
        res.push_back(lepus_event_handler);
      }
    }
    return res;
  };

  Element *target = *response_chain.begin();
  if (!option.lepus_event_) {
    for (const auto &current_target : response_chain) {
      if (current_target && current_target->EnableTriggerGlobalEvent()) {
        operation.emplace_back(nullptr, target, current_target, true);
      }
    }
  }

  const auto &push_global_bind_operation = [&event_name, &operation,
                                            &get_handler_f](Element *cur_target,
                                                            Element *target) {
    auto handlers = get_handler_f(cur_target, event_name, true);
    for (auto handler : handlers) {
      // Need to copy rather than ref because the handler may be a null pointer
      if (handler != nullptr) {
        operation.emplace_back(handler, target, cur_target, false);
      }
    }
  };

  const auto &handle_global_bind_target =
      [&push_global_bind_operation](
          Element *cur_target, Element *target,
          const std::set<std::string> &global_bind_targets) {
        for (const auto &id_selector : global_bind_targets) {
          // if set not empty, means the target should have not empty id,
          // when data_model is null pointer or element id is empty, then not
          // send event
          if (target->data_model() == nullptr ||
              target->data_model()->idSelector().empty()) {
            continue;
          }
          if (id_selector == target->data_model()->idSelector().str()) {
            push_global_bind_operation(cur_target, target);
          }
        }
      };
  ElementManager *manager = target->element_manager();
  if (manager->GetGlobalBindElementIds(event_name).size() > 0) {
    for (const auto &id : manager->GetGlobalBindElementIds(event_name)) {
      Element *cur_target = node_manager_->Get(id);
      auto set = cur_target->GlobalBindTarget();
      if (set.empty()) {
        // if set is empty, means the target is all other elements
        push_global_bind_operation(cur_target, target);
      } else {
        if (set.size() > 0) {
          if (option.bubbles_) {
            for (const auto &target : response_chain) {
              handle_global_bind_target(cur_target, target, set);
            }
          } else {
            handle_global_bind_target(cur_target, target, set);
          }
        }
      }
    }
  }

  bool consume = false;
  bool capture = false;
  if (option.capture_phase_) {
    Element *cur_target = nullptr;
    for (auto iter = response_chain.rbegin(); iter != response_chain.rend();
         ++iter) {
      cur_target = *iter;
      if (cur_target == nullptr) break;
      auto handlers = get_handler_f(cur_target, event_name, false);
      bool need_break = false;
      for (auto handler : handlers) {
        // Need to copy rather than ref because the handler may be a null
        // pointer
        if (!handler) {
          continue;
        }
        if (handler->IsCaptureCatchEvent()) {
          operation.emplace_back(handler, target, cur_target, false);
          capture = true;
          consume = true;
          // Set need_break to true to exit the outer loop
          need_break = true;
        } else if (handler->IsCaptureBindEvent()) {
          operation.emplace_back(handler, target, cur_target, false);
          consume = true;
        }
      }
      if (need_break) {
        // Exit the outer loop if need_break is true
        break;
      }
    }
  }

  if (!capture) {
    for (auto *cur_target : response_chain) {
      if (cur_target == nullptr) break;
      auto handlers = get_handler_f(cur_target, event_name, false);
      bool need_break = false;
      for (auto handler : handlers) {
        // Need to copy rather than ref because the handler may be a null
        // pointer
        if (!handler) continue;
        if (handler->IsCatchEvent()) {
          operation.emplace_back(handler, target, cur_target, false);
          consume = true;
          // Set need_break to true to exit the outer loop
          need_break = true;
        } else if (handler->IsBindEvent()) {
          operation.emplace_back(handler, target, cur_target, false);
          consume = true;
          if (!option.bubbles_) {
            if (option.from_frontend_ && cur_target != response_chain.front()) {
              report::FeatureCounter::Instance()->Count(
                  report::LynxFeature::CPP_FE_CUSTOM_EVENT_BUBBLE_BUG);
            }
            // Set need_break to true to exit the outer loop
            need_break = true;
          }
        }
      }
      if (need_break) {
        // Exit the outer loop if need_break is true
        break;
      }
    }  // for
  }    // if
  return consume;
}

std::string TouchEventHandler::GetEventType(const EventType &type) const {
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

void TouchEventHandler::SendPageEvent(const EventType &type,
                                      const std::string &page_name,
                                      const std::string &event_name,
                                      const std::string &handler,
                                      const lepus::Value &info) const {
  LOGI("SendPageEvent " << GetEventType(type) << ": " << event_name
                        << " with function: " << handler);
  auto args = lepus::CArray::Create();
  args->emplace_back(page_name);
  args->emplace_back(handler);
  // info be ShallowCopy first to avoid to be marked const.
  args->emplace_back(lepus_value::ShallowCopy(info));
  runtime::MessageEvent event(runtime::kMessageEventTypeSendPageEvent,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value(std::move(args)));
  context_proxy_delegate_.DispatchMessageEvent(std::move(event));
  if (type != EventType::kComponent) {
    constexpr const static char *kPrefix = "Page";
    tasm::replay::ReplayController::SendFileByAgent(
        kPrefix + GetEventType(type),
        tasm::replay::ReplayController::ConvertEventInfo(info));
  }
}

void TouchEventHandler::PublishComponentEvent(const EventType &type,
                                              const std::string &component_id,
                                              const std::string &event_name,
                                              const std::string &handler,
                                              const lepus::Value &info) const {
  LOGI("PublishComponentEvent " << GetEventType(type) << ": " << event_name
                                << " with function: " << handler);

  auto args = lepus::CArray::Create();
  args->emplace_back(component_id);
  args->emplace_back(handler);
  // info be ShallowCopy first to avoid to be marked const.
  args->emplace_back(lepus_value::ShallowCopy(info));
  runtime::MessageEvent event(runtime::kMessageEventTypePublishComponentEvent,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value(std::move(args)));
  context_proxy_delegate_.DispatchMessageEvent(std::move(event));
  if (type != EventType::kComponent) {
    constexpr const static char *kPrefix = "Component";
    tasm::replay::ReplayController::SendFileByAgent(
        kPrefix + GetEventType(type),
        tasm::replay::ReplayController::ConvertEventInfo(info));
  }
}

void TouchEventHandler::SendGlobalEvent(const EventType &type,
                                        const std::string &name,
                                        const lepus::Value &info) const {
  LOGI("SendGlobalEvent " << GetEventType(type) << ": " << name);
  auto args = lepus::CArray::Create();
  args->emplace_back(name);
  // info be ShallowCopy first to avoid to be marked const.
  args->emplace_back(lepus_value::ShallowCopy(info));
  runtime::MessageEvent event(runtime::kMessageEventTypeSendGlobalEvent,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value(std::move(args)));
  context_proxy_delegate_.DispatchMessageEvent(std::move(event));
  if (type != EventType::kComponent) {
    constexpr const static char *kPrefix = "Global";
    tasm::replay::ReplayController::SendFileByAgent(
        kPrefix + GetEventType(type),
        tasm::replay::ReplayController::ConvertEventInfo(info));
  }
}

void TouchEventHandler::TriggerLepusBridgesAsync(
    const EventType &type, TemplateAssembler *tasm,
    const std::string &event_name,
    const std::vector<PiperEventContent> &piper_event_vec) const {
  for (auto &event : piper_event_vec) {
    const auto &func_name = event.piper_func_name_.str();
    auto func_args = event.piper_func_args_;
    LOGI("TriggerPiperEventAsync " << GetEventType(type) << ": " << event_name
                                   << " with function: " << func_name);

    tasm->TriggerLepusBridgeAsync(func_name, func_args);
    if (type != EventType::kComponent) {
      constexpr const static char *kPrefix = "Bridge";
      tasm::replay::ReplayController::SendFileByAgent(
          kPrefix + GetEventType(type),
          tasm::replay::ReplayController::ConvertEventInfo(func_args));
    }
  }
}

void TouchEventHandler::EnsureGestureManager(lepus::Context *context) {
  // Ensure that the context is valid and the gesture_manager_ is empty
  if (context && gesture_manager_.IsEmpty()) {
    // Create a new object for gesture_manager_ using the provided context
    gesture_manager_ = lepus::Value::CreateObject(context);

    // Register method to the gesture manager using utility function
    tasm::Utils::RegisterNGMethodToGestureManager(context, gesture_manager_);
  }
}

// This method is used to call the front-end framework layer entry function in
// Fiber scenarios.
std::optional<lepus::Value> TouchEventHandler::TriggerFiberElementWorklet(
    tasm::TemplateAssembler *tasm, const lepus::Value &worklet_info,
    const lepus::Value &event_param, int element_id, tasm::EventType type,
    lepus::Context *context) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "TouchEventHandler::TriggerFiberElementWorklet");

  if (tasm == nullptr) {
    LOGE(
        "TouchEventHandler::TriggerFiberElementWorklet failed since tasm "
        "is null.");
    return std::nullopt;
  }

  if (worklet_info.IsEmpty()) {
    LOGE(
        "TouchEventHandler::TriggerFiberElementWorklet failed since "
        "worklet_info is empty "
        "is null.");
    return std::nullopt;
  }

  if (context == nullptr) {
    LOGE(
        "TouchEventHandler::TriggerFiberElementWorklet failed since "
        "QuickContext "
        "is null.");
    return std::nullopt;
  }

  constexpr const static char kEntryFunction[] = "runWorklet";

  // Get the worklet function value
  const auto worklet_function_value =
      context->GetGlobalData(BASE_STATIC_STRING(kEntryFunction));
  auto param_array = lepus::CArray::Create();
  param_array->push_back(event_param);

  if (!gesture_manager_.IsEmpty() && type == tasm::EventType::kGesture) {
    param_array->push_back(gesture_manager_);
  }

  // Call the worklet function with closure
  lepus::Value call_result_value =
      context->CallClosure(worklet_function_value, worklet_info,
                           lepus::Value(std::move(param_array)));

  return call_result_value;
}

EventResult TouchEventHandler::FireElementWorklet(
    EventContext &context, const std::string &component_id,
    const std::string &entry_name, tasm::TemplateAssembler *tasm,
    EventHandler *handler, const lepus::Value &value, int element_id) const {
  EventResult result = EventResult::kDefault;
  if (tasm && tasm->EnableFiberArch()) {
    // trigger worklet in fiber
    LOGI("Fire Fiber Element Worklet " << GetEventType(context.event_type)
                                       << ": " << context.event_name);
    TriggerFiberElementWorklet(tasm, handler->lepus_object(), value, element_id,
                               context.event_type, handler->lepus_context());
  } else {
#if ENABLE_LEPUSNG_WORKLET
    LOGI("FireLepusEvent " << GetEventType(context.event_type) << ": "
                           << context.event_name);
    result = lynx::worklet::LepusElement::FireElementWorklet(
        component_id, entry_name, tasm, handler->lepus_function(),
        handler->lepus_script(), value, task_handler_, element_id,
        context.event_type);
    // trigger patch finish when a worklet operation is completed
    tasm::PipelineOptions options;
    // TODO(kechenglong): SetNeedsLayout if and only if needed.
    tasm->page_proxy()->element_manager()->SetNeedsLayout();
    tasm->page_proxy()->element_manager()->OnPatchFinish(options);
#endif  // ENABLE_LEPUSNG_WORKLET
  }
  if (context.event_type != EventType::kComponent) {
    constexpr const static char *kPrefix = "Lepus";
    tasm::replay::ReplayController::SendFileByAgent(
        kPrefix + GetEventType(context.event_type),
        tasm::replay::ReplayController::ConvertEventInfo(value));
  }
  return result;
}

}  // namespace tasm
}  // namespace lynx

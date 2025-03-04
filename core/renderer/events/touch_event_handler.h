// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_EVENTS_TOUCH_EVENT_HANDLER_H_
#define CORE_RENDERER_EVENTS_TOUCH_EVENT_HANDLER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/page_proxy.h"
#include "core/runtime/bindings/common/event/context_proxy.h"

namespace lynx {
namespace worklet {
class LepusApiHandler;
class LepusComponent;

}  // namespace worklet
namespace tasm {

class AttributeHolder;
class NodeManager;
class BaseComponent;
class RadonPage;
class RadonComponent;
class RadonNode;
class TemplateAssembler;

// This struct is used to record event-related operations. After execing
// HandleEventInternal function, the event-related operations will be stored in
// a stc::vector<EventOperation>.
struct EventOperation {
  EventOperation(EventHandler *handler, Element *target,
                 Element *current_target, bool global_event)
      : handler_(handler),
        target_(target),
        current_target_(current_target),
        global_event_(global_event) {}
  EventHandler *handler_;
  Element *target_;
  Element *current_target_;
  bool global_event_;

  bool IsSameTargetAndEventPhase(const EventOperation &that) const {
    return this->current_target_ == that.current_target_ &&
           this->handler_->GetEventPhase() == that.handler_->GetEventPhase();
  }
};

struct EventInfo {
  explicit EventInfo(const lepus_value &params, int64_t timestamp = 0)
      : params(params), is_multi_finger(true), timestamp(timestamp) {}
  EventInfo(int tag, float x, float y, float client_x, float client_y,
            float page_x, float page_y, int64_t timestamp = 0)
      : tag(tag),
        x(x),
        y(y),
        client_x(client_x),
        client_y(client_y),
        page_x(page_x),
        page_y(page_y),
        timestamp(timestamp) {}
  EventInfo(const EventInfo &info) = delete;
  EventInfo &operator=(const EventInfo &info) = delete;
  EventInfo(EventInfo &&info) noexcept = default;
  EventInfo &operator=(EventInfo &&info) noexcept = default;

  lepus_value params;
  bool is_multi_finger{false};
  int32_t tag{0};
  float x{0};
  float y{0};
  float client_x{0};
  float client_y{0};
  float page_x{0};
  float page_y{0};
  int64_t timestamp{0};
};

enum class EventType : unsigned int {
  kTouch = 1,
  kCustom = 2,
  kComponent = 3,
  kBubble = 4,
  kGesture = 5
};

enum class EventResult : int {
  kDefault = 0x0,
  kStopPropagation = 0x1,
  kStopImmediatePropagation = 0x2
};

class TouchEventHandler {
 public:
  TouchEventHandler(NodeManager *node_manager,
                    runtime::ContextProxy::Delegate &context_proxy_delegate,
                    bool support_component_js, bool use_lepus_ng,
                    const std::string &version);

  // TODO(songshourui.null) : unify the following three functions.
  void HandleTouchEvent(TemplateAssembler *tasm, const std::string &page_name,
                        const std::string &name, const EventInfo &info);

  void HandleBubbleEvent(TemplateAssembler *tasm, const std::string &page_name,
                         const std::string &event_name, int tag,
                         lepus::DictionaryPtr params);

  void HandleTriggerComponentEvent(TemplateAssembler *tasm,
                                   const std::string &event_name,
                                   const lepus::Value &data);
  // in lepus event, this is used to call js function
  void CallJSFunctionInLepusEvent(const std::string &component_id,
                                  const std::string &name,
                                  const lepus::Value &params);
  // in lepus event, this is used to callback js function return value
  void HandleJSCallbackLepusEvent(const int64_t callback_id,
                                  TemplateAssembler *tasm,
                                  const lepus::Value &data);

  void HandleCustomEvent(TemplateAssembler *tasm, const std::string &name,
                         int tag, const lepus::Value &params,
                         const std::string &pname);
  void HandlePseudoStatusChanged(int32_t id, PseudoState pre_status,
                                 PseudoState current_status);
  static lepus::Value GetTargetInfo(int32_t impl_id,
                                    const AttributeHolder *holder,
                                    const Element *element = nullptr,
                                    bool is_js_event = true);

  void HandleGestureEvent(TemplateAssembler *tasm, const base::String &name,
                          int tag, int gesture_id, const lepus::Value &params);

 private:
  struct EventContext {
    EventType event_type;
    std::string event_name;
    std::string page_name;
    EventOption option;
    std::function<lepus::Value(Element *, Element *, bool is_js_event)>
        get_event_params;

    EventContext(const EventContext &info) = delete;
    EventContext &operator=(const EventContext &info) = delete;
    EventContext(EventContext &&info) noexcept = default;
    EventContext &operator=(EventContext &&info) noexcept = default;
  };

  using ResponseChainVector = base::InlineVector<Element *, 16>;
  using EventOpsVector = base::InlineVector<EventOperation, 2>;

  ResponseChainVector GenerateResponseChain(int tag, const EventOption &option);
  ResponseChainVector GenerateResponseChain(PageProxy *proxy,
                                            Element *component_element,
                                            const EventOption &option);

  void FireTouchEvent(const std::string &page_name, const EventHandler *handler,
                      const Element *target, const Element *current_target,
                      const EventInfo &info);

  void ApplyEventTargetParams(lepus::DictionaryPtr params,
                              const Element *target,
                              const Element *currentTarget,
                              bool is_js_event) const;
  void FireEvent(const EventType &type, const std::string &page_name,
                 const EventHandler *handler, const Element *target,
                 const Element *current_target,
                 const lepus::Value &params) const;

  void FireEventForAir(TemplateAssembler *tasm, const EventType &type,
                       const std::string &page_name,
                       const EventHandler *handler, const Element *target,
                       const Element *current_target,
                       const lepus::Value &params) const;

  void FireTriggerComponentEvent(PageProxy *proxy, const EventHandler *handler,
                                 Element *target, Element *current_target,
                                 const lepus::Value &data);

  lepus::Value GetCustomEventParam(const std::string &name,
                                   const std::string &pname,
                                   const EventOption &option, Element *target,
                                   Element *current_target,
                                   const lepus::Value &data,
                                   bool is_js_event) const;

  lepus::Value GetTouchEventParam(const base::String &handler,
                                  const Element *target,
                                  const Element *current_target,
                                  const EventInfo &info,
                                  bool is_js_event) const;

  lepus::Value GetTouchEventParam(const base::String &handler,
                                  const Element *target,
                                  const Element *currentTarget, float x,
                                  float y, float client_x, float client_y,
                                  float page_x, float page_y, bool is_js_event,
                                  int64_t timestamp = 0) const;

  lepus_value GetTouchEventParam(const base::String &handler,
                                 const Element *target,
                                 const Element *current_target,
                                 const lepus_value &params, bool is_js_event,
                                 int64_t timestamp = 0) const;

  bool HandleEventInternal(const ResponseChainVector &response_chain,
                           const std::string &event_name,
                           const EventOption &option,
                           EventOpsVector &operation);

  std::string GetEventType(const EventType &type) const;

  void SendPageEvent(const EventType &type, const std::string &page_name,
                     const std::string &event_name, const std::string &handler,
                     const lepus::Value &info) const;
  void PublishComponentEvent(const EventType &type,
                             const std::string &component_id,
                             const std::string &event_name,
                             const std::string &handler,
                             const lepus::Value &info) const;
  void SendGlobalEvent(const EventType &type, const std::string &name,
                       const lepus::Value &info) const;
  void TriggerLepusBridgesAsync(
      const EventType &type, TemplateAssembler *tasm,
      const std::string &event_name,
      const std::vector<PiperEventContent> &piper_event_vec) const;

  EventResult FireElementWorklet(
      EventContext &context, const std::string &component_id,
      const std::string &entry_name, tasm::TemplateAssembler *tasm,
      EventHandler *handler, const lepus::Value &value, int element_id) const;

  void HandleEventOperations(TemplateAssembler *tasm, EventContext &context,
                             const EventOpsVector &ops);
  // This method is used to call the front-end framework layer entry function in
  // Fiber scenarios.
  std::optional<lepus::Value> TriggerFiberElementWorklet(
      tasm::TemplateAssembler *tasm, const lepus::Value &worklet_info,
      const lepus::Value &event_param, int element_id, tasm::EventType type,
      lepus::Context *context) const;

  // Register method to the gesture manager using utility function
  void EnsureGestureManager(lepus::Context *context);

  NodeManager *node_manager_;

  runtime::ContextProxy::Delegate &context_proxy_delegate_;
  bool support_component_js_;
  bool long_press_consumed_{false};

  bool use_lepus_ng_{false};
  std::string version_;

  lepus::Value current_touches_;

  // for new gesture in fiber
  lepus::Value gesture_manager_;

#if ENABLE_LEPUSNG_WORKLET
 public:
  // TODO(wangyang.ryan): Decoupe task_handler with touch
  const std::shared_ptr<worklet::LepusApiHandler> &GetTaskHandler() const {
    return task_handler_;
  }

 private:
  std::shared_ptr<worklet::LepusApiHandler> task_handler_;
#endif  // ENABLE_LEPUSNG_WORKLET
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_EVENTS_TOUCH_EVENT_HANDLER_H_

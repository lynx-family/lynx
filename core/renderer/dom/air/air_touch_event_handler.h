// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_TOUCH_EVENT_HANDLER_H_
#define CORE_RENDERER_DOM_AIR_AIR_TOUCH_EVENT_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/template_assembler.h"

namespace lynx {
namespace tasm {

class AirNodeManager;
class TemplateAssembler;

// This struct is used to record event-related operations. After execing
// HandleEventInternal function, the event-related operations will be stored in
// std::vector<AirEventOperation>.
struct AirEventOperation {
  AirEventOperation(EventHandler *handler, AirElement *target,
                    AirElement *current_target, bool global_event)
      : handler(handler),
        target(target),
        current_target(current_target),
        global_event(global_event) {}
  EventHandler *handler;
  AirElement *target;
  AirElement *current_target;
  bool global_event;
};

class AirTouchEventHandler : public AirTouchEventHandlerBase {
 public:
  AirTouchEventHandler(AirNodeManager *air_node_manager);

  /**
   * HandleTouchEvent handle touch event
   */
  void HandleTouchEvent(TemplateAssembler *tasm, const std::string &page_name,
                        const std::string &name, int tag, float x, float y,
                        float client_x, float client_y, float page_x,
                        float page_y) override;

  /**
   * HandleCustomEvent customEvent for example: x-element's custom event
   */
  void HandleCustomEvent(TemplateAssembler *tasm, const std::string &name,
                         int tag, const lepus::Value &params,
                         const std::string &pname) override;

  /**
   * SendPageEvent air life function and global event
   */
  void SendPageEvent(TemplateAssembler *tasm, const std::string &handler,
                     const lepus::Value &info) const override;

  /**
   * SendComponentEvent send Component related lifecycle event
   */
  void SendComponentEvent(TemplateAssembler *tasm,
                          const std::string &event_name, const int component_id,
                          const lepus::Value &params,
                          const std::string &param_name) override;

  size_t TriggerComponentEvent(TemplateAssembler *tasm,
                               const std::string &event_name,
                               const lepus::Value &data) override;

 private:
  enum class EventType { kTouch, kCustom, kComponent, kBubble };

  std::vector<AirElement *> GenerateResponseChain(int tag,
                                                  const EventOption &option,
                                                  bool componentEvent = false);

  void FireTouchEvent(TemplateAssembler *tasm, const std::string &page_name,
                      const EventHandler *handler, const AirElement *target,
                      const AirElement *current_target, float x, float y,
                      float client_x, float client_y, float page_x,
                      float page_y);

  void HandleEventOperation(TemplateAssembler *tasm, const std::string &name,
                            const lepus::Value &params,
                            const std::string &pname, const EventOption &option,
                            const std::vector<AirEventOperation> &ops);

  lepus::Value GetCustomEventParam(const std::string &name,
                                   const std::string &pname,
                                   const EventOption &option,
                                   AirElement *target,
                                   AirElement *current_target,
                                   const lepus::Value &data) const;

  static lepus::Value GetTargetInfo(const AirElement *target);

  lepus::Value GetTouchEventParam(const base::String &handler,
                                  const AirElement *target,
                                  const AirElement *current_target, float x,
                                  float y, float client_x, float client_y,
                                  float page_x, float page_y) const;

  bool GenerateEventOperation(const std::vector<AirElement *> &response_chain,
                              const std::string &event_name,
                              const EventOption &option,
                              std::vector<AirEventOperation> &operation);

  std::string GetEventType(const EventType &type) const;

  void SendPageEvent(TemplateAssembler *tasm, const EventType &type,
                     const std::string &page_name,
                     const std::string &event_name, const std::string &handler,
                     const lepus::Value &info, const AirElement *target) const;

  void SendComponentEvent(TemplateAssembler *tasm, const EventType &type,
                          const int component_id, const std::string &event_name,
                          const std::string &handler, const lepus::Value &info,
                          const AirElement *target) const;

  void SendBaseEvent(TemplateAssembler *tasm, const std::string &event_name,
                     const std::string &handler, const lepus::Value &info,
                     const AirElement *target) const;

  const AirElement *GetComponentTarget(TemplateAssembler *tasm,
                                       const AirElement *target,
                                       bool ignore_target = true) const;

  EventHandler *GetEventHandler(AirElement *cur_target,
                                const std::string &event_name);

  std::vector<AirEventOperation> GetEventOperation(
      const std::string &event_name, const std::vector<AirElement *> &chain,
      const EventOption &option, bool &long_press_consumed);

  AirNodeManager *air_node_manager_;
  bool long_press_consumed_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_TOUCH_EVENT_HANDLER_H_

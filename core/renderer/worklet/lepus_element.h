// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_WORKLET_LEPUS_ELEMENT_H_
#define CORE_RENDERER_WORKLET_LEPUS_ELEMENT_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/events/events.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/bindings/napi/worklet/napi_func_callback.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {

namespace tasm {
class Element;
}  // namespace tasm

namespace worklet {
class LepusComponent;
class LepusGesture;

using binding::BridgeBase;
using binding::ImplBase;

class LepusElement : public ImplBase {
 public:
  static LepusElement* Create(
      int32_t element_id, const std::shared_ptr<tasm::TemplateAssembler>& tasm,
      const std::shared_ptr<worklet::LepusApiHandler>& task_handler) {
    return new LepusElement(element_id, tasm, task_handler);
  }
  static tasm::EventResult FireElementWorklet(
      const std::string& component_id, const std::string& entry_name,
      tasm::TemplateAssembler* tasm, lepus::Value& func_val,
      const lepus::Value& func_obj, const lepus::Value& value,
      const std::shared_ptr<worklet::LepusApiHandler>& task_handler,
      int element_id, tasm::EventType type);

  static std::optional<lepus::Value> TriggerWorkletFunction(
      tasm::TemplateAssembler* tasm, tasm::BaseComponent* component,
      const std::string& worklet_module_name, const std::string& method_name,
      const lepus::Value& args,
      const std::shared_ptr<worklet::LepusApiHandler>& task_handler);

  static std::optional<lepus::Value> CallLepusWithComponentInstance(
      lynx::tasm::TemplateAssembler* tasm, LEPUSContext* ctx,
      const LEPUSValue& func_obj, const LEPUSValue& this_obj,
      const LEPUSValue& args, const LEPUSValue& component_instance,
      const std::unique_ptr<LEPUSValue> gesture_instance);

  LepusElement(const LepusElement&) = delete;
  ~LepusElement() override = default;

  tasm::Element* GetElement();

  void SetStyles(const Napi::Object& styles);
  void SetAttributes(const Napi::Object& attributes);

  Napi::Object GetComputedStyles(const std::vector<Napi::String>& keys);
  Napi::Object GetAttributes(const std::vector<Napi::String>& keys);
  Napi::Object GetDataset();

  // Function
  Napi::Value ScrollBy(float width, float height);
  Napi::Value GetBoundingClientRect();
  void Invoke(const Napi::Object& object);

 private:
  static void ReportPendingJobException(lynx::tasm::TemplateAssembler* tasm,
                                        LEPUSContext* ctx, bool is_ur);

  LepusElement(int32_t element_id,
               const std::shared_ptr<tasm::TemplateAssembler>& tasm,
               const std::shared_ptr<worklet::LepusApiHandler>& task_handler);
  int32_t element_id_{-1};
  std::weak_ptr<tasm::TemplateAssembler> weak_tasm_;
  std::weak_ptr<LepusApiHandler> task_handler_;
};
}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RENDERER_WORKLET_LEPUS_ELEMENT_H_

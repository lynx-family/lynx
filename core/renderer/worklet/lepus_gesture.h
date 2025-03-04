// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_WORKLET_LEPUS_GESTURE_H_
#define CORE_RENDERER_WORKLET_LEPUS_GESTURE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/template_assembler.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_raf_handler.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {

namespace tasm {
class TemplateAssembler;
}  // namespace tasm

namespace worklet {

using binding::BridgeBase;
using binding::ImplBase;

class NapiFrameCallback;
class NapiFuncCallback;

class LepusGesture : public ImplBase {
 public:
  // Factory method to create a new LepusGesture instance
  static LepusGesture* Create(
      int32_t element_id,
      const std::shared_ptr<tasm::TemplateAssembler>& tasm) {
    return new LepusGesture(element_id, tasm);
  }

  // Constructor, copy constructor is deleted
  LepusGesture(const LepusGesture&) = delete;

  // Destructor, set as default (no custom cleanup needed)
  ~LepusGesture() override = default;

  // This enum represents a gesture, which can have one of four states: START,
  // ACTIVE, END, or FAIL.
  enum class LynxGestureState : unsigned int {
    ACTIVE = 1,  // The gesture is in progress.
    FAIL = 2,    // The gesture has failed to complete.
    END = 3,     // The gesture has ended successfully.
  };

  void Active(Napi::Number gesture_id);

  void Fail(Napi::Number gesture_id);

  void End(Napi::Number gesture_id);

  // Scroll the view by the given delta values and return the new position.
  // Parameters:
  // - deltaX: the horizontal distance to scroll
  // - deltaY: the vertical distance to scroll
  // Returns: a Napi::Value representing the new position of the view

  Napi::Value ScrollBy(float delta_x, float delta_y);

 private:
  LepusGesture(int32_t element_id,
               const std::shared_ptr<tasm::TemplateAssembler>& tasm);

  tasm::Element* GetElement();

  int32_t element_id_{-1};

  std::weak_ptr<tasm::TemplateAssembler> weak_tasm_;
};

}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RENDERER_WORKLET_LEPUS_GESTURE_H_

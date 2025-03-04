// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/worklet/lepus_gesture.h"

namespace lynx {
namespace worklet {

// Constructor
LepusGesture::LepusGesture(int32_t element_id,
                           const std::shared_ptr<tasm::TemplateAssembler>& tasm)
    : element_id_(element_id), weak_tasm_(tasm) {}

// Set the gesture state to ACTIVE
void LepusGesture::Active(Napi::Number gesture_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusGesture::Active");
  auto element = GetElement();
  if (element != nullptr) {
    element->SetGestureDetectorState(
        gesture_id.Int32Value(),
        static_cast<int32_t>(LynxGestureState::ACTIVE));
  } else {
    LOGE("LepusGesture::Active failed, since element is null.");
  }
}

// Set the gesture state to FAIL
void LepusGesture::Fail(Napi::Number gesture_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusGesture::Fail");
  auto element = GetElement();
  if (element != nullptr) {
    element->SetGestureDetectorState(
        gesture_id.Int32Value(), static_cast<int32_t>(LynxGestureState::FAIL));
  } else {
    LOGE("LepusGesture::Fail failed, since element is null.");
  }
}

// Set the gesture state to END
void LepusGesture::End(Napi::Number gesture_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusGesture::End");
  auto element = GetElement();
  if (element != nullptr) {
    element->SetGestureDetectorState(
        gesture_id.Int32Value(), static_cast<int32_t>(LynxGestureState::END));
  } else {
    LOGE("LepusGesture::End failed, since element is null.");
  }
}

// Scroll the view by the given delta values and return the new position.
// Parameters:
// - width: the horizontal distance to scroll
// - height: the vertical distance to scroll
// Returns: a Napi::Value representing the new position of the view
Napi::Value LepusGesture::ScrollBy(float width, float height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusGesture::ScrollBy");
  auto env = NapiEnv();
  Napi::Object obj = Napi::Object::New(env);
  std::vector<float> res{0, 0, width, height};
  auto element = GetElement();

  if (element != nullptr) {
    res = element->ScrollBy(width * element->computed_css_style()
                                        ->GetMeasureContext()
                                        .layouts_unit_per_px_,
                            height * element->computed_css_style()
                                         ->GetMeasureContext()
                                         .layouts_unit_per_px_);
  } else {
    LOGE("LepusGesture::ScrollBy failed, since element is null.");
  }

  // Constants for the properties in the returned Napi::Object
  constexpr const static char* kConsumedX = "consumedX";
  constexpr const static char* kConsumedY = "consumedY";
  constexpr const static char* kUnConsumedX = "unconsumedX";
  constexpr const static char* kUnConsumedY = "unconsumedY";

  // Set the properties in the Napi::Object with the values from 'res'
  obj.Set(kConsumedX, res[0] / element->computed_css_style()
                                   ->GetMeasureContext()
                                   .layouts_unit_per_px_);
  obj.Set(kConsumedY, res[1] / element->computed_css_style()
                                   ->GetMeasureContext()
                                   .layouts_unit_per_px_);
  obj.Set(kUnConsumedX, res[2] / element->computed_css_style()
                                     ->GetMeasureContext()
                                     .layouts_unit_per_px_);
  obj.Set(kUnConsumedY, res[3] / element->computed_css_style()
                                     ->GetMeasureContext()
                                     .layouts_unit_per_px_);

  return obj;
}

// Get the associated Element from the TemplateAssembler
tasm::Element* LepusGesture::GetElement() {
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    return nullptr;
  }
  if (tasm->destroyed()) {
    return nullptr;
  }
  return tasm->page_proxy()->element_manager()->node_manager()->Get(
      element_id_);
}

}  // namespace worklet

}  // namespace lynx

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/platform_layout_function_wrapper.h"

#include <utility>

#include "core/public/platform_extra_bundle.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace tasm {

PlatformLayoutFunctionWrapper::PlatformLayoutFunctionWrapper(
    FiberElement& element, const fml::RefPtr<PropBundle>& initial_props)
    : element_(element) {
  layout_object_ = element.slnode();
  if (layout_object_) {
    id_ = element.impl_id();
  }
}

FloatSize PlatformLayoutFunctionWrapper::MeasureCallback(
    void* context, const starlight::Constraints& constraints,
    bool final_measure) {
  MeasureFunc* measure = (static_cast<PlatformLayoutFunctionWrapper*>(context))
                             ->measure_func_.get();
  DCHECK(measure);
  SLMeasureMode width_mode = constraints[starlight::kHorizontal].Mode();
  SLMeasureMode height_mode = constraints[starlight::kVertical].Mode();
  float width = IsSLIndefiniteMode(width_mode)
                    ? 0.f
                    : constraints[starlight::kHorizontal].Size();
  float height = IsSLIndefiniteMode(height_mode)
                     ? 0.f
                     : constraints[starlight::kVertical].Size();

  LayoutResult result =
      measure->Measure(width, width_mode, height, height_mode, final_measure);

  return FloatSize(result.width_, result.height_, result.baseline_);
}

void PlatformLayoutFunctionWrapper::SetMeasureFunc(
    std::unique_ptr<MeasureFunc> measure_func) {
  measure_func_ = std::move(measure_func);
  layout_object_->SetContext(this);
  layout_object_->SetSLMeasureFunc(MeasureCallback);
}

void PlatformLayoutFunctionWrapper::MarkDirty() {
  if (layout_object_) {
    layout_object_->MarkDirty();
  }
}

void PlatformLayoutFunctionWrapper::UpdateLayoutNodeProps(
    const fml::RefPtr<PropBundle>& props) {
  element_.element_manager()->layout_context()->UpdateLayoutNode(id_,
                                                                 props.get());
}

void PlatformLayoutFunctionWrapper::Destroy() {
  element_.element_manager()->DestroyLayoutNode(id_);
  measure_func_ = nullptr;
}

void PlatformLayoutFunctionWrapper::OnLayoutBefore() {
  element_.element_manager()->layout_context()->OnLayoutBefore(
      element_.impl_id());
}

}  // namespace tasm
void tasm::PlatformLayoutFunctionWrapper::OnLayoutAfter() {
  // if node has custom measure function, it may by need pass some bundle to
  auto bundle =
      element_.element_manager()->layout_context()->GetPlatformExtraBundle(
          element_.impl_id());

  if (!bundle) {
    return;
  }

  element_.element_manager()->painting_context()->UpdatePlatformExtraBundle(
      element_.impl_id(), bundle.get());
}
}  // namespace lynx

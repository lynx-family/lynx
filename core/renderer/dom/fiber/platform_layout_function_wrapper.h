// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_PLATFORM_LAYOUT_FUNCTION_WRAPPER_H_
#define CORE_RENDERER_DOM_FIBER_PLATFORM_LAYOUT_FUNCTION_WRAPPER_H_

#include <memory>

#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace tasm {

class FiberElement;

class PlatformLayoutFunctionWrapper {
 public:
  explicit PlatformLayoutFunctionWrapper(
      FiberElement& element, const fml::RefPtr<PropBundle>& initial_props);
  ~PlatformLayoutFunctionWrapper() = default;

  static FloatSize MeasureCallback(void* context,
                                   const starlight::Constraints& constraints,
                                   bool final_measure);

  void SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func);
  void UpdateLayoutNodeProps(const fml::RefPtr<PropBundle>& props);
  void MarkDirty();
  void Destroy();
  void OnLayoutBefore();
  void OnLayoutAfter();

 private:
  FiberElement& element_;
  std::unique_ptr<MeasureFunc> measure_func_;
  starlight::LayoutObject* layout_object_ = nullptr;
  int32_t id_ = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_PLATFORM_LAYOUT_FUNCTION_WRAPPER_H_

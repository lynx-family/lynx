// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_CUSTOM_LAYOUT_H_
#define CORE_RENDERER_DOM_FIBER_CUSTOM_LAYOUT_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/public/layout_node_value.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}
namespace tasm {

class Element;

// ComposeElementHandle owns this external policy adapter. Elements retain only
// their existing platform measurement hooks, including for nested policies.
class CustomLayout final : public std::enable_shared_from_this<CustomLayout> {
 public:
  CustomLayout(Element& owner, runtime::MTSRuntime& runtime,
               const lepus::Value& measure_callback,
               const lepus::Value& place_callback);
  CustomLayout(const CustomLayout&) = delete;
  CustomLayout& operator=(const CustomLayout&) = delete;
  void visitor(void* rt, void* func, uint64_t trace_tool) const;
  LayoutResult Measure(float width, int32_t width_mode, float height,
                       int32_t height_mode, bool final_measure);
  void Alignment();

  bool MeasureChild(Element& child, float min_width, float max_width,
                    float min_height, float max_height, LayoutResult& result);
  bool PlaceChild(Element& child, float x, float y, float z);
  bool IsExecuting() const { return phase_ != Phase::kIdle; }
  void Invalidate();
  Element& owner() const { return *owner_; }
  void Detach();

 private:
  enum class Phase { kIdle, kMeasure, kPlace };
  struct Child {
    fml::RefPtr<Element> element;
    bool placed = false;
    float x = 0;
    float y = 0;
    float z = 0;
    size_t order = 0;
  };
  bool IsChild(Element& child) const;
  void ResetPlacementOrder();

  static thread_local CustomLayout* active_;
  fml::RefPtr<Element> owner_;
  runtime::MTSRuntime& runtime_;
  lepus::Value measure_callback_;
  lepus::Value place_callback_;
  Phase phase_ = Phase::kIdle;
  bool final_measure_ = false;
  bool valid_ = false;
  size_t placement_order_ = 0;
  std::vector<Child> children_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_CUSTOM_LAYOUT_H_

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_MODIFIER_SCROLL_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_MODIFIER_SCROLL_ELEMENT_H_

#include <cstdint>
#include <string>

#include "core/renderer/dom/fiber/modifier_scroll_state.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/starlight/types/layout_constraints.h"

namespace lynx {
namespace tasm {

// Compose owns input and animation. This boundary measures content and owns the
// synchronous offset; the ordinary ScrollElement retains platform scrolling.
class ModifierScrollElement : public ScrollElement {
 public:
  explicit ModifierScrollElement(ElementManager* manager);
  bool is_modifier() const override { return true; }

  void Configure(int binding_id, bool vertical, bool reverse, bool rtl,
                 float initial_offset_px);
  int binding_id() const { return binding_id_; }
  void ResetModifierStyles();

  lepus::Value InvokeMethod(const base::String& method,
                            const lepus::Value& params,
                            std::string* error) override;
  fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const override;

 private:
  ModifierScrollElement(const ModifierScrollElement& element,
                        bool clone_resolved_props);
  void InstallLayoutFunctions();
  FloatSize Measure(const starlight::ComposeConstraints& constraints,
                    bool final_measure);
  void Align();
  void OnSubtreeLayoutUpdated(PipelineOptions*) override;
  void Project();
  lepus::Value StateValue(double consumed_px = 0, double remaining_px = 0);

  int binding_id_{0};
  bool vertical_{true};
  bool reverse_{false};
  bool rtl_{false};
  bool bounded_viewport_{true};
  bool has_projection_{false};
  ModifierScrollState scroll_state_;
  float projected_content_width_{0};
  float projected_content_height_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_MODIFIER_SCROLL_ELEMENT_H_

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;

class TemplateElement : public Element {
 public:
  explicit TemplateElement(ElementManager* element_manager = nullptr);
  ~TemplateElement() override;

  fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const override {
    return fml::AdoptRef<Element>(
        new TemplateElement(*this, clone_resolved_props));
  }

  bool is_template() const override { return true; }
  void SetTASM(TemplateAssembler* tasm) { tasm_ = tasm; }
  void SetTypedTag(const base::String& typed_tag);
  bool IsTypedTemplate() const { return !typed_tag_.empty(); }
  void SetRootAttributes(const lepus::Value& attributes);
  void SetElementSlots(const lepus::Value& element_slots);

  fml::RefPtr<Element> GetRoot();
  fml::RefPtr<Element> GetResolvedRoot() const { return result_; }

 private:
  TemplateElement(const TemplateElement& element, bool clone_resolved_props)
      : Element(element, clone_resolved_props),
        tasm_(element.tasm_),
        typed_tag_(element.typed_tag_),
        root_attributes_(element.root_attributes_),
        element_slots_(element.element_slots_) {}

  void ResolveGeneratedElements();
  void InitTypedRoot();
  void ApplyRootAttributes(const lepus::Value& previous_root_attributes);
  void ApplyInitialElementSlots();
  bool IsActiveMaterialized() const { return result_ != nullptr; }
  void InsertInitialElementSlotChild(const ElementSlotMountPoint& mount_point,
                                     const fml::RefPtr<Element>& child);
  TemplateAssembler* tasm_{nullptr};
  base::String typed_tag_;
  lepus::Value root_attributes_;
  lepus::Value element_slots_;
  fml::RefPtr<Element> result_{nullptr};
  base::Vector<ElementSlotMountPoint> element_slot_targets_;
  base::Vector<PreparedElementSlotInsertion> prepared_element_slot_insertions_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

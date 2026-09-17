// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

#include <memory>
#include <utility>

#include "core/base/thread/once_task.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;

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
  void SetTemplateKey(const base::String& template_key) {
    template_key_ = template_key;
  }
  void SetBundleUrl(const base::String& bundle_url) {
    bundle_url_ = bundle_url;
  }
  void SetTypedTag(const base::String& typed_tag);
  bool IsTypedTemplate() const { return !typed_tag_.empty(); }
  void SetRootAttributes(const lepus::Value& attributes);
  void SetAttributeSlots(const lepus::Value& attribute_slots) {
    attribute_slots_ = attribute_slots;
  }
  void SetElementSlots(const lepus::Value& element_slots);

  void PrepareAsyncCreateElementTree();
  fml::RefPtr<Element> GetRoot();
  fml::RefPtr<Element> GetResolvedRoot() const { return result_; }

 private:
  enum class TemplateElementTreeState {
    kDetached,
    kInTemplateTree,
  };

  TemplateElement(const TemplateElement& element, bool clone_resolved_props)
      : Element(element, clone_resolved_props),
        tasm_(element.tasm_),
        entry_(element.entry_),
        template_key_(element.template_key_),
        bundle_url_(element.bundle_url_),
        typed_tag_(element.typed_tag_),
        root_attributes_(element.root_attributes_),
        root_attributes_generation_(element.root_attributes_generation_),
        attribute_slots_(element.attribute_slots_),
        element_slots_(element.element_slots_) {}

  // Builds a once task that may run either on the concurrent loop or on the
  // GetRoot caller. The task returns detached data; GetRoot is the only place
  // that moves the result back onto TemplateElement and initializes the tree.
  base::OnceTaskRefptr<GeneratedElementsResult>
  CreateAsyncCreateElementTreeTask(TemplateEntry* entry);
  void ResolveGeneratedElements();
  void InitGeneratedElementTree(const lepus::Value& prepared_root_attributes,
                                uint32_t prepared_root_attributes_generation);
  void ApplyInitialRootEventAttributes(
      const lepus::Value& prepared_root_attributes,
      uint32_t prepared_root_attributes_generation);
  void InitTypedRoot();
  bool IsPageTemplate() const;
  void MarkInTemplateTreeAndPrepare();
  void MarkInTemplateTreeAndPrepareRecursively();
  void MarkTemplateChildrenInElementSlotsInTree();
  void ApplyRootAttributes(const lepus::Value& previous_root_attributes);
  void ApplyInitialElementSlots();
  bool IsInTemplateTree() const {
    return template_tree_state_ == TemplateElementTreeState::kInTemplateTree;
  }
  bool IsActiveMaterialized() const { return result_ != nullptr; }
  void InsertInitialElementSlotChild(const ElementSlotMountPoint& mount_point,
                                     const fml::RefPtr<Element>& child);
  TemplateAssembler* tasm_{nullptr};
  TemplateEntry* entry_{nullptr};
  base::String template_key_;
  base::String bundle_url_;
  base::String typed_tag_;
  lepus::Value root_attributes_;
  uint32_t root_attributes_generation_{0};
  lepus::Value attribute_slots_;
  lepus::Value element_slots_;
  fml::RefPtr<Element> result_{nullptr};
  base::Vector<fml::RefPtr<Element>> attribute_slot_targets_;
  base::Vector<fml::RefPtr<Element>> event_attribute_slot_targets_;
  base::Vector<fml::RefPtr<Element>> static_event_targets_;
  base::Vector<ElementSlotMountPoint> element_slot_targets_;
  base::Vector<PreparedElementSlotInsertion> prepared_element_slot_insertions_;
  base::OnceTaskRefptr<GeneratedElementsResult> async_create_task_{nullptr};
  TemplateElementTreeState template_tree_state_{
      TemplateElementTreeState::kDetached};
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

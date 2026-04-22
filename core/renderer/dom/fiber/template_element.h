// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

#include <memory>

#include "core/base/thread/once_task.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;

class TemplateElement : public FiberElement {
 public:
  explicit TemplateElement(ElementManager* element_manager = nullptr);
  ~TemplateElement() override;

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
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
  void SetAttributeSlots(const lepus::Value& attribute_slots) {
    attribute_slots_ = attribute_slots;
  }
  void SetElementSlots(const lepus::Value& element_slots) {
    element_slots_ = element_slots;
  }
  void SetUid(const lepus::Value& uid) { uid_ = uid; }

  void PrepareAsyncCreateElementTree();
  fml::RefPtr<FiberElement> GetRoot();
  lepus::Value Serialize() const;

 private:
  TemplateElement(const TemplateElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props),
        tasm_(element.tasm_),
        entry_(element.entry_),
        template_key_(element.template_key_),
        bundle_url_(element.bundle_url_),
        attribute_slots_(element.attribute_slots_),
        element_slots_(element.element_slots_),
        uid_(element.uid_) {}

  // Builds a once task that may run either on the concurrent loop or on the
  // GetRoot caller. The task returns detached data; GetRoot is the only place
  // that moves the result back onto TemplateElement and initializes the tree.
  base::OnceTaskRefptr<GeneratedElementsResult>
  CreateAsyncCreateElementTreeTask(TemplateEntry* entry);
  void ResolveGeneratedElements();
  void InitGeneratedElementTree();
  void ApplyInitialElementSlots();
  void InsertInitialElementSlotChild(const ElementSlotMountPoint& mount_point,
                                     const fml::RefPtr<FiberElement>& child);
  lepus::Value SerializeElementSlots() const;
  lepus::Value SerializeElementSlotChildren(
      const lepus::Value& slot_children) const;
  lepus::Value SerializeElementSlotChild(const lepus::Value& child) const;

  TemplateAssembler* tasm_{nullptr};
  TemplateEntry* entry_{nullptr};
  base::String template_key_;
  base::String bundle_url_;
  lepus::Value attribute_slots_;
  lepus::Value element_slots_;
  lepus::Value uid_;
  fml::RefPtr<FiberElement> result_{nullptr};
  base::Vector<fml::RefPtr<FiberElement>> attribute_slot_targets_;
  base::Vector<ElementSlotMountPoint> element_slot_targets_;
  base::Vector<PreparedElementSlotInsertion> prepared_element_slot_insertions_;
  base::OnceTaskRefptr<GeneratedElementsResult> async_create_task_{nullptr};
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

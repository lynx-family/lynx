// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_LIST_UPDATE_INFO_RECORDER_H_
#define CORE_RENDERER_DOM_FIBER_LIST_UPDATE_INFO_RECORDER_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fiber/element_template_instance.h"

namespace lynx {
namespace tasm {

class Element;

class ListUpdateInfoRecorder {
 public:
  struct InsertBeforeGroup {
    // Retains the anchor so its instance pointer remains valid as a map key
    // while operations are recorded.
    fml::RefPtr<ElementTemplateInstance> ref_child_;
    // Retains items inserted before this anchor in call order.
    base::Vector<fml::RefPtr<ElementTemplateInstance>> children_;
  };

  using InsertBeforeMap =
      std::unordered_map<ElementTemplateInstance*, InsertBeforeGroup>;

  explicit ListUpdateInfoRecorder(const lepus::Value& old_children);

  // Resolves Values at the entry point; the recorder only stores strong
  // references to ET instances.
  void OnInsertBefore(const lepus::Value& child, const lepus::Value& ref_child);

  // Records a logical item for removal without modifying the child slot or
  // triggering materialization.
  void OnRemoveChild(const lepus::Value& child);

  // Stores the item's latest platform metadata, replacing any previous record
  // for the same item.
  void OnSetAttribute(const lepus::Value& child,
                      const lepus::Value& platform_info);

  // Generates a diff from the old children and recorded operations. A true
  // result indicates that the recorder for this batch can be released.
  // Call once at the common commit point; Element::FlushActions then consumes
  // the attribute.
  bool Flush(Element* list_element);

  const base::Vector<fml::RefPtr<ElementTemplateInstance>>& old_children()
      const {
    return old_children_;
  }

  const InsertBeforeMap& insert_before() const { return insert_before_; }

  const base::Vector<fml::RefPtr<ElementTemplateInstance>>& append_child()
      const {
    return append_child_;
  }

  const std::unordered_set<fml::RefPtr<ElementTemplateInstance>>& remove_child()
      const {
    return remove_child_;
  }

  const std::unordered_map<fml::RefPtr<ElementTemplateInstance>, lepus::Value>&
  platform_info_update() const {
    return platform_info_update_;
  }

 private:
  // Example: "__Card__:_et_e807c3550368"
  static std::string GetListItemType(const ElementTemplateInstance& instance);

  // Example: "__Card__:_et_e807c3550368#-4"
  static std::string GetListItemTypeUID(
      const ElementTemplateInstance& instance);

 private:
  // Independent snapshot of the order before this batch, with no shared storage
  // with the child slot array that subsequent operations modify.
  base::Vector<fml::RefPtr<ElementTemplateInstance>> old_children_;
  // Groups operations by anchor instance identity. Different items using the
  // same template are still distinct anchors.
  InsertBeforeMap insert_before_;
  // Append operations without an anchor. Like the groups, this records
  // operations rather than the final item order.
  base::Vector<fml::RefPtr<ElementTemplateInstance>> append_child_;
  // Deduplicates by instance identity and retains items marked for removal
  // until this batch's records are released.
  std::unordered_set<fml::RefPtr<ElementTemplateInstance>> remove_child_;
  // Retains items and stores platform metadata in separate objects so later
  // changes do not affect this batch's records.
  std::unordered_map<fml::RefPtr<ElementTemplateInstance>, lepus::Value>
      platform_info_update_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_LIST_UPDATE_INFO_RECORDER_H_

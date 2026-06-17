// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_GENERATED_ELEMENTS_RESULT_H_
#define CORE_RENDERER_DOM_FIBER_GENERATED_ELEMENTS_RESULT_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

struct ElementSlotMountPoint {
  fml::RefPtr<FiberElement> parent_;
  fml::RefPtr<FiberElement> ref_node_;
};

struct PreparedElementSlotInsertion {
  uint32_t slot_index_{0};
  fml::RefPtr<FiberElement> child_;
};

// Aggregated output of one Element Template materialization task.
//
// The task always produces a detached single-root tree plus two slot target
// tables:
// - attribute_slot_targets_[i] records the element affected by attrSlotIndex=i
// - event_attribute_slot_targets_[i] records targets whose runtime slot may
//   contain event attrs that need listener replay after attach
// - element_slot_targets_[i] records where elementSlotIndex=i should mount into
//   the generated tree
// - static_event_targets_ records elements whose static event attrs need
//   listener replay after the generated tree is attached
//
// The result is returned from the async task first, then moved into
// TemplateElement on the consuming thread.
struct GeneratedElementsResult {
  fml::RefPtr<FiberElement> result_;
  base::Vector<fml::RefPtr<FiberElement>> attribute_slot_targets_;
  base::Vector<fml::RefPtr<FiberElement>> event_attribute_slot_targets_;
  base::Vector<fml::RefPtr<FiberElement>> static_event_targets_;
  base::Vector<ElementSlotMountPoint> element_slot_targets_;
  base::Vector<PreparedElementSlotInsertion> prepared_element_slot_insertions_;
  lepus::Value prepared_root_attributes_;
  uint32_t root_attributes_generation_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_GENERATED_ELEMENTS_RESULT_H_

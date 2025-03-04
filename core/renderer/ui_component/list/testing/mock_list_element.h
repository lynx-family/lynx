// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_LIST_ELEMENT_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_LIST_ELEMENT_H_

#include "core/renderer/dom/fiber/list_element.h"

namespace lynx {
namespace tasm {
namespace list {

class MockListElement : public ListElement {
 public:
  MockListElement(ElementManager* manager, const base::String& tag,
                  const lepus::Value& component_at_index,
                  const lepus::Value& enqueue_component,
                  const lepus::Value& component_at_indexes)
      : ListElement(manager, tag, component_at_index, enqueue_component,
                    component_at_indexes) {}

  int32_t ComponentAtIndex(uint32_t index, int64_t operationId,
                           bool enable_reuse_notification) override {
    return -1;
  }

  void EnqueueComponent(int32_t sign) {}
};

}  // namespace list
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_LIST_ELEMENT_H_

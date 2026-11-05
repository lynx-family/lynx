// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_TESTING_ELEMENT_TEMPLATE_INSTANCE_TEST_H_
#define CORE_RENDERER_DOM_TESTING_ELEMENT_TEMPLATE_INSTANCE_TEST_H_

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <vector>

#include "core/renderer/dom/fiber/element_template_instance.h"
#include "core/renderer/dom/testing/fiber_element_test.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}  // namespace runtime

namespace tasm {
namespace testing {

class ElementTemplateInstanceTest : public FiberElementTest {
 protected:
  // Create a JS runtime with tasm as its delegate.
  std::shared_ptr<runtime::MTSRuntime> CreatePAPIRuntime();

  fml::RefPtr<ElementTemplateInstance> CreateCompiledSpreadInstance(
      bool nested_target = false);

  fml::RefPtr<ElementTemplateInstance> CreateCompiledCallbackInstance(
      const std::shared_ptr<runtime::MTSRuntime>& runtime, bool nested_target,
      const char* key = "main-thread:gesture");

  lepus::Value GestureDescriptor(std::initializer_list<int> ids,
                                 int callback_version, int entry_count = 1);

  void CreateCSSEntry(const char* name, const char* default_color,
                      const char* scoped_color);

  fml::RefPtr<ElementTemplateInstance> CreateCSSInstance(const char* entry,
                                                         bool scoped);

  void ExpectCSSColor(ElementTemplateInstance* instance, uint32_t color);

  std::vector<fml::RefPtr<ElementTemplateInstance>> CreateCompiledMountChain(
      size_t length);

  void ExpectPageFlushCompletesNestedAttrSlotMaterialization(
      bool query_before_insertion);
};

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_TESTING_ELEMENT_TEMPLATE_INSTANCE_TEST_H_

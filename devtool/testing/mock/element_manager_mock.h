// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_ELEMENT_MANAGER_MOCK_H_
#define DEVTOOL_TESTING_MOCK_ELEMENT_MANAGER_MOCK_H_

#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace testing {

class ElementManagerMock : public tasm::ElementManager {
 public:
  ElementManagerMock()
      : tasm::ElementManager(nullptr, nullptr,
                             tasm::LynxEnvConfig(100, 100, 0, 0)) {}

  bool IsDomTreeEnabled() override { return dom_tree_enabled_; }
  bool dom_tree_enabled_ = true;
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_ELEMENT_MANAGER_MOCK_H_

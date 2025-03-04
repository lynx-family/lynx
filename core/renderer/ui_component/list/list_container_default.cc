// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_container_default.h"

#include <memory>

namespace lynx {
namespace tasm {

namespace list {
std::unique_ptr<ListContainer::Delegate> CreateListContainerDelegate(
    Element* element) {
  return std::make_unique<ListContainerDefault>();
}
}  // namespace list

}  // namespace tasm
}  // namespace lynx

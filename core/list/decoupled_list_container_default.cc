// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_container_default.h"

namespace lynx {
namespace list {

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory) {
  return std::make_unique<ListContainerDefault>(list_delegate, value_factory);
}

}  // namespace list
}  // namespace lynx

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/list/decoupled_list_container_factory.h"
#include "core/list/decoupled_list_container_impl.h"

namespace lynx {
namespace list {

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory) {
  return std::make_unique<ListContainerImpl>(list_delegate, value_factory,
                                             CreateListAnimationManager,
                                             CreateAnimationManager);
}

}  // namespace list
}  // namespace lynx

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_CONTAINER_FACTORY_H_
#define CORE_LIST_DECOUPLED_LIST_CONTAINER_FACTORY_H_

#include <memory>

#include "core/list/list_container_delegate.h"

namespace lynx {
namespace list {

// Creates an animated C++ list container when the C++ list implementation is
// enabled, or a default container otherwise. Provided by list_container.
std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory);

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_CONTAINER_FACTORY_H_

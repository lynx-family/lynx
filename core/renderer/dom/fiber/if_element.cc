// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/if_element.h"

namespace lynx {
namespace tasm {

void IfElement::UpdateIfIndex(int32_t ifIndex) {
  if (static_cast<uint32_t>(ifIndex) == active_index_) {
    return;
  }

  RemoveAllBlockNodes();
  active_index_ = ifIndex;
}

}  // namespace tasm
}  // namespace lynx

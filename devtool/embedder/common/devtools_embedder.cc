// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/common/devtools_embedder.h"

#include <utility>

#include "core/renderer/data/template_data.h"
#include "devtool/embedder/core/inspector_owner_embedder.h"

namespace lynx {
namespace devtool {

DevtoolsEmbedder::DevtoolsEmbedder(devtool::LynxDevToolProxy* proxy) {
  owner_ = std::make_shared<InspectorOwnerEmbedder>();
  owner_->Init(proxy, owner_);
}

void DevtoolsEmbedder::SetInputEventTarget(
    const std::shared_ptr<input::InputEventTarget>& target) {
  std::static_pointer_cast<InspectorOwnerEmbedder>(owner_)->SetInputEventTarget(
      target);
}

}  // namespace devtool
}  // namespace lynx

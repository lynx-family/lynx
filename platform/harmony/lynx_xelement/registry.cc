// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/registry.h"

#include "platform/harmony/lynx_xelement/input/input_shadow_node.h"
#include "platform/harmony/lynx_xelement/input/ui_input.h"
#include "platform/harmony/lynx_xelement/input/ui_textarea.h"

namespace lynx {
namespace tasm {
namespace harmony {

void XElementRegistry::Initialize() {
  auto& map = LynxContext::GetCAPINodeInfoMap();
  map["input"] = {UIInput::Make, InputShadowNode::Make, LayoutNodeType::CUSTOM};
  map["textarea"] = {UITextArea::Make, InputShadowNode::Make,
                     LayoutNodeType::CUSTOM};
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

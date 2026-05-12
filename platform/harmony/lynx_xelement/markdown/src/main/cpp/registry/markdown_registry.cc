// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/registry/markdown_registry.h"

#include <string>

#include "base/include/log/logging.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

#if defined(LYNX_XELEMENT_MARKDOWN_BUILTIN)
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/shadow_node/markdown_shadow_node.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/ui/ui_markdown.h"
#endif

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

#if defined(LYNX_XELEMENT_MARKDOWN_BUILTIN)
UIBase* MarkdownUICreator(LynxContext* context, int sign,
                          const std::string& tag) {
  return UIMarkdown::Make(context, sign, tag);
}

ShadowNode* MarkdownShadowCreator(int sign, const std::string& tag) {
  return MarkdownShadowNode::Make(sign, tag);
}
#endif

}  // namespace

void MarkdownRegistry::Initialize() {
#if defined(LYNX_XELEMENT_MARKDOWN_BUILTIN)
  auto& map = LynxContext::GetCAPINodeInfoMap();
  map["x-markdown"] = {MarkdownUICreator, MarkdownShadowCreator,
                       LayoutNodeType::CUSTOM};
  LOGI("x-markdown: registered native XElement built into liblynx");
#else
  LOGI("x-markdown: native XElement is not built");
#endif
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_CONTENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_CONTENT_H_

#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ParagraphContent {
 public:
  virtual void OnAppendToParagraph(ParagraphBuilderHarmony& builder,
                                   float width, float height) = 0;
  virtual void AppendToParagraph(ParagraphBuilderHarmony& builder,
                                 float width = .0f, float height = .0f) = 0;
  virtual ~ParagraphContent() = default;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_CONTENT_H_

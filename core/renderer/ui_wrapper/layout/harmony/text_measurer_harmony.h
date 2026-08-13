// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_MEASURER_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_MEASURER_HARMONY_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "base/include/fml/memory/ref_counted.h"
#include "core/public/layout_node_value.h"

namespace lynx {
namespace tasm {

class Element;

namespace harmony {
class FontCollectionHarmony;
class LynxContext;
class ParagraphHarmony;
}  // namespace harmony

class TextMeasurerHarmony final {
 public:
  explicit TextMeasurerHarmony(harmony::LynxContext* context);
  ~TextMeasurerHarmony();

  void DispatchLayoutBefore(Element* element);
  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode);
  void Align(Element* element);
  void Destroy(Element* element);

  fml::RefPtr<harmony::ParagraphHarmony> GetParagraph(int32_t id) const;

 private:
  harmony::LynxContext* context_{nullptr};
  std::shared_ptr<harmony::FontCollectionHarmony> font_collection_;
  std::unordered_map<int32_t, fml::RefPtr<harmony::ParagraphHarmony>>
      paragraphs_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_TEXT_MEASURER_HARMONY_H_

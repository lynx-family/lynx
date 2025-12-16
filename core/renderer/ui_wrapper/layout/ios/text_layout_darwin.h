// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_IOS_TEXT_LAYOUT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_IOS_TEXT_LAYOUT_DARWIN_H_

#import <Foundation/Foundation.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxUIOwner.h>

#include "core/public/text_layout_impl.h"
#include "core/renderer/starlight/types/layout_constraints.h"

namespace lynx {
namespace tasm {

class FiberElement;
class TextElement;

class TextLayoutDarwin : public TextLayoutImpl {
 public:
  explicit TextLayoutDarwin(LynxUIOwner* uiOwner) : uiOwner_(uiOwner) {}

  ~TextLayoutDarwin() override = default;

  LayoutResult Measure(Element* element, float width, int width_mode, float height,
                       int height_mode) override;

  void Align(Element* element) override;

  void DispatchLayoutBefore(Element* element) override;

 private:
  static void ApplyTextStyle(TextElement* text_element, LynxTextStyle* textStyle);
  static void HandleParagraphStyle(TextElement* text_element, LynxTextStyle* textStyle,
                                   LynxAttributedTextBundle* textBundle);
  void GenerateAttributedString(NSMutableAttributedString* attributedString, Element* element,
                                NSDictionary<NSAttributedStringKey, id>* baseAttributes,
                                NSMutableSet* inlineElementSigns, Boolean* hasViewOrImage);
  void ProcessChildAttribute(NSMutableAttributedString* attributedString, FiberElement* element,
                             NSDictionary<NSAttributedStringKey, id>* baseAttributes,
                             NSMutableSet* inlineElementSigns, Boolean* hasViewOrImage);
  void MeasureChildrenRecursively(Element* element, const starlight::Constraints& constraints,
                                  bool final_measure, NSMutableDictionary* layoutResultDic);

  void AlignChildrenRecursively(Element* element, NSDictionary* offsetDic);

  __weak LynxUIOwner* uiOwner_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_IOS_TEXT_LAYOUT_DARWIN_H_

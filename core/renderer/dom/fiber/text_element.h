// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_

#include <memory>
#include <string>

#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/text_props.h"

namespace lynx {
namespace tasm {

class TextElement : public FiberElement {
 public:
  TextElement(ElementManager* manager, const base::String& tag);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new TextElement(*this, clone_resolved_props));
  }

  bool is_text() const override { return true; }
  void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value,
                        bool force_update = false) override;
  void ConvertToInlineElement() override;

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  bool ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value,
                         bool force_update) override;

  bool ResetCSSValue(CSSPropertyID id) override;

  void ResetAttribute(const base::String& key) override;

  LayoutResult Measure(float width, int32_t width_mode, float height,
                       int32_t height_mode, bool final_measure);

  void Align();

  void OnLayoutObjectCreated() override;

  void UpdateLayoutNodeFontSize(double cur_node_font_size,
                                double root_node_font_size) override;

  void DispatchLayoutBefore() override;

  TextProps* text_props() { return text_props_.get(); };

  base::String& content() { return content_; };

  void set_need_layout_children(bool value) { need_layout_children_ = value; }

  bool need_layout_children() { return need_layout_children_; }

  bool has_inline_child() { return has_inline_child_; }

  size_t content_utf16_length() { return content_utf16_length_; }

  CSSIDBitset& property_bits() { return property_bits_; }

  int32_t GetBuiltInNodeInfo() const override {
    return is_inline_element() ? kVirtualBuiltInNodeInfo
                               : kCommonBuiltInNodeInfo;
  }

 protected:
  void OnNodeAdded(FiberElement* child) override;
  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  static base::String ConvertContent(const lepus::Value);

  TextElement(const TextElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

 private:
  void ResolveAndFlushFontFaces(const base::String& font_family);
  bool ProcessAttributeForLayoutInElement(const base::String& key,
                                          const lepus::Value& value,
                                          bool is_reset = false);
  bool ProcessAttributeForNormalLayoutMode(const base::String& key,
                                           const lepus::Value& value);

  void EnsureTextProps() {
    if (!text_props_) {
      text_props_ = std::make_unique<TextProps>();
    }
  }

  base::String content_;
  // TODO(linxs): Use base::String.length_utf16() after its implementation has
  // been optimized
  size_t content_utf16_length_{0};
  std::unique_ptr<TextProps> text_props_;
  CSSIDBitset property_bits_;
  bool has_inline_child_{false};
  bool need_layout_children_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_

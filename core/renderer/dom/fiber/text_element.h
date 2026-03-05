// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_

#include <memory>
#include <utility>

#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/text_props.h"

namespace lynx {
namespace tasm {

class Fragment;

// Line layout information for text layout event
struct TextLineInfo {
  int start{0};
  int end{0};
  int ellipsis_count{0};
};

// Type alias for TextLineInfo array (using unique_ptr with count)
using TextLineInfoArray = std::unique_ptr<TextLineInfo[]>;
class TextElement : public FiberElement {
 public:
  TextElement(ElementManager* manager, const base::String& tag);
  ~TextElement() override;

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new TextElement(*this, clone_resolved_props));
  }

  bool is_text() const override { return true; }
  bool NeedProcessDirection() override { return true; }

  void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value) override;

  void ConvertToInlineElement() override;

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  bool ResolveStyleValue(CSSPropertyID id,
                         const tasm::CSSValue& value) override;

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

  void SetTextBundle(intptr_t text_bundle);

  void SetTextLineLayoutInfo(TextLineInfoArray line_infos, int line_count) {
    line_layout_info_ = std::move(line_infos);
    line_layout_count_ = line_count;
  }

  const TextLineInfo* GetTextLineLayoutInfo() const {
    return line_layout_info_.get();
  }

  int GetTextLineLayoutCount() const { return line_layout_count_; }

  void ClearTextLineLayoutInfo() {
    line_layout_info_.reset();
    line_layout_count_ = 0;
  }

 protected:
  void OnNodeAdded(FiberElement* child) override;
  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  static base::String ConvertContent(const lepus::Value);

  TextElement(const TextElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

  void SetupFragmentBehavior(Fragment* fragment) override;

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

  // Line layout information from text measurement (null-terminated array)
  TextLineInfoArray line_layout_info_;
  int line_layout_count_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_

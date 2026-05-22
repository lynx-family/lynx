// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_INTERNAL_TEXT_VIEW_H_
#define CLAY_UI_COMPONENT_TEXT_INTERNAL_TEXT_VIEW_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/measurable.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"
#include "clay/ui/rendering/text/render_text.h"
#if defined(CLAY_ENABLE_SKSHAPER)
#include "clay/third_party/txt/src/skia/paragraph_skia.h"
#endif
#if defined(CLAY_ENABLE_MINIKIN)
#include "clay/third_party/txt/src/txt/paragraph.h"
#endif

namespace clay {

// without shadow node and need measure、layout  by parent component
class InternalTextView : public BaseView, public Measurable {
 public:
  explicit InternalTextView(int id, PageView* page_view);
  explicit InternalTextView(int id, const std::string& tag,
                            std::unique_ptr<RenderObject> render_object,
                            PageView* page_view);
  ~InternalTextView();

  const std::u16string& text() { return text_; }

  bool ShouldCreateStyle();
  void SetEnableFontScaling(bool enabled);
  void SetFontSize(float font_size);
  void SetLineHeight(float line_height);
  // TODO(Xietong): Not supported by the underlining layout engine.
  void SetLineSpacing(float line_spacing);
  void SetLetterSpacing(float letter_spacing);
  void SetTextAlign(TextAlignment text_align);
  void SetFontWeight(FontWeight font_weight);
  void SetFontStyle(FontStyle font_style);
  void SetTextColor(const Color& text_color);
  void SetTextBackgroundColor(const Color& color);
  void SetFontFamily(const std::string& font_family);
  std::optional<TextDecoration> GetTextDecoration() const {
    return text_style_ ? std::nullopt : text_style_->text_decoration;
  }
  void SetTextDecoration(const TextDecoration& text_decoration);
  void AppendTextShadow(Shadow&& text_shadow);
  void SetTextShadows(std::vector<Shadow>&& text_shadows);
  void SetTextGradient(const Gradient& gradient);
  void SetTextMaxLine(uint32_t max_lines);
  void SetTextMaxLength(uint32_t max_length);
  void SetTextOverflow(TextOverflow overflow);
  void SetTextEllipsis(std::u16string ellipsis);
  void SetText(const std::string& text);

  RenderText* GetRenderText();

  void OnLayout(LayoutContext* context) override;
  void Measure(const MeasureConstraint& constraint,
               MeasureResult& result) override;

  void DidUpdateStyle();
  void SetUseSkia(bool use_skia) { use_skia_ = use_skia; }

  void AddTapUpListener(
      std::function<void(const PointerEvent& down_event)> func);

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 protected:
  float LayoutWidth() { return ContentWidth(); }
  bool use_skia_ = true;

 private:
  void UpdateDefaultFontSize();
  float GetDefaultFontSize() const;
  std::unique_ptr<txt::Paragraph> paragraph_;

  TextStyle default_style_;

  enum UpdateFlag {
    // None means either there is no update or the text view's width/height
    // changed.
    // In this case, we don't need to re-create the text builder.
    kUpdateFlagNone = 0,
    kUpdateFlagStyle = 1,
    kUpdateFlagChildren = 1 << 1,
  };
  UpdateFlag update_flag_ = kUpdateFlagNone;
  float prev_layout_width_ = 0;

  std::optional<TextStyle> text_style_;
  std::optional<uint32_t> max_length_;

  std::u16string text_;
  TapGestureRecognizer* tap_recognizer_ = nullptr;

  FRIEND_TEST(ViewPagerLegacyTest, BuiltinTabBar_TextStyles);
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_INTERNAL_TEXT_VIEW_H_

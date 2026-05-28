// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_API_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_API_H_
#include <cstdint>
#include <memory>

#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/fiber/text_props.h"
#include "core/renderer/starlight/style/css_type.h"
namespace lynx {
namespace tasm {
namespace text {
class Paragraph;
class Page;
enum class LayoutMode : uint8_t { kIndefinite, kDefinite, kAtMost };
struct MeasureResult {
  float width;
  float height;
  float baseline;
};
struct MeasureParams {
  float width;
  LayoutMode width_mode;
  float height;
  LayoutMode height_mode;
};
class InlineView {
 public:
  virtual ~InlineView() = default;
  virtual MeasureResult Measure(const MeasureParams &params) = 0;
  virtual void Align(float x, float y) = 0;
  virtual void HideView() = 0;
};

struct Radius {
  float top_left;
  int top_left_type;  // 0:number, 1:percentage
  float top_right;
  int top_right_type;
  float bottom_left;
  int bottom_left_type;
  float bottom_right;
  int bottom_right_type;
};

struct VerticalAlign {
  int vertical_align;
  float vertical_align_length;
};

struct ImageProps {
  float width;
  float height;
  int margin_left;
  int margin_top;
  int margin_right;
  int margin_bottom;
  Radius radius;
};

struct AutoFontSize {
  bool enabled;
  float min_size;
  float max_size;
  float step_granularity;
};

struct AutoFontSizeLineRange {
  int32_t start_line;
  int32_t end_line;
  float min_size;
  float max_size;
};

enum TextEventTargetMask : uint32_t {
  kTextEventTargetTap = 1u << 0,
  kTextEventTargetClick = 1u << 1,
  kTextEventTargetLongPress = 1u << 2,
  kTextEventTargetTouchStart = 1u << 3,
  kTextEventTargetTouchMove = 1u << 4,
  kTextEventTargetTouchEnd = 1u << 5,
  kTextEventTargetTouchCancel = 1u << 6,
};

struct EventTargetInfo {
  int32_t sign{0};
  uint32_t event_mask{0};
  bool is_inline_view{false};

  bool IsValid() const {
    return sign != 0 && (event_mask != 0 || is_inline_view);
  }
};

class ParagraphListener {
 public:
  virtual ~ParagraphListener() = default;
  virtual void MarkParagraphDirty() = 0;
};

class ParagraphBuilder {
 public:
  virtual ~ParagraphBuilder() = default;
  virtual void SetParagraphListener(ParagraphListener *listener) = 0;
  virtual void SetParagraphStyle(TextPropertyKeyID key, void *value,
                                 size_t length) = 0;
  virtual void PushTextStyle() = 0;
  virtual void PopTextStyle() = 0;
  virtual void PushEventTarget(const EventTargetInfo &event_target) = 0;
  virtual void PopEventTarget() = 0;
  virtual void SetTextStyle(TextPropertyKeyID key, void *value,
                            size_t length) = 0;
  virtual void AddText(const char *text, size_t length) = 0;
  virtual void AddInlineView(std::unique_ptr<InlineView> inline_view) = 0;
  virtual void AddImage(const char *src, size_t length) = 0;
  virtual void SetPlaceHolderStyle(TextPropertyKeyID key, void *value,
                                   size_t length) = 0;
  virtual Paragraph *BuildParagraph() = 0;
};

class TextLayoutAPI {
 public:
  virtual ~TextLayoutAPI() = default;
  virtual ParagraphBuilder *CreateParagraphBuilder() = 0;
  virtual void DestroyParagraphBuilder(ParagraphBuilder *builder) = 0;
  virtual MeasureResult MeasureParagraph(Paragraph *paragraph,
                                         MeasureParams params) = 0;
  virtual void AlignParagraph(Paragraph *paragraph, float x, float y) = 0;
  virtual Page *GetPage(Paragraph *paragraph) = 0;
  virtual void DestroyPage(Page *page) = 0;
  virtual void DestroyParagraph(Paragraph *paragraph) = 0;
};
}  // namespace text
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXTRA_TEXT_LAYOUT_API_H_

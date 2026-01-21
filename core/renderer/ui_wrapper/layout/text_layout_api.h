// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXT_LAYOUT_API_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXT_LAYOUT_API_H_
#include <cstdint>
#include <memory>

#include "core/renderer/dom/fiber/text_props.h"
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
};
class ParagraphListener {
 public:
  virtual ~ParagraphListener() = 0;
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
  virtual void SetTextStyle(TextPropertyKeyID key, void *value,
                            size_t length) = 0;
  virtual void AddText(const char *text) = 0;
  virtual void AddInlineView(std::unique_ptr<InlineView> inline_view) = 0;
  virtual void AddImage(const char *src) = 0;
  virtual void SetPlaceHolderStyle(TextPropertyKeyID key, void *value,
                                   size_t length) = 0;
  virtual Paragraph *BuildParagraph() = 0;
};
class TextLayoutAPI {
 public:
  ParagraphBuilder *CreateParagraphBuilder();
  void DestroyParagraphBuilder(ParagraphBuilder *builder);
  MeasureResult MeasureParagraph(Paragraph *paragraph);
  void AlignParagraph(Paragraph *paragraph, float x, float y);
  Page *GetPage(Paragraph *paragraph);
  void DestroyPage(Page *page);
  void DestroyParagraph(Paragraph *paragraph);
};
}  // namespace text
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_TEXT_LAYOUT_API_H_

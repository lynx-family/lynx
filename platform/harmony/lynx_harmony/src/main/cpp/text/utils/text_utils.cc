// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/text_utils.h"

#include <limits>
#include <memory>

#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
lepus::Value TextUtils::GetTextInfo(const std::string& content,
                                    const pub::Value& info, LynxContext* ctx) {
  lepus::Value result(lepus::Dictionary::Create());
  float width = 0.f;
  if (content.empty() || !info.IsMap() || !info.Contains(kFontSize) ||
      ctx == nullptr) {
    result.SetProperty(BASE_STATIC_STRING(kWidth), lepus::Value(width));
    return result;
  }

  float scale_density = ctx->ScaledDensity();
  float screen_size[2] = {0};
  ctx->ScreenSize(screen_size);
  const std::string& font_size_str = info.GetValueForKey(kFontSize)->str();
  float font_size = LynxUnitUtils::ToVPFromUnitValue(
                        font_size_str, screen_size[0], ctx->DevicePixelRatio(),
                        ctx->DefaultFontSize()) *
                    scale_density;

  float max_width = std::numeric_limits<float>::max();
  if (info.Contains(kMaxWidth)) {
    const std::string& max_width_str = info.GetValueForKey(kMaxWidth)->str();
    if (!max_width_str.empty()) {
      max_width = LynxUnitUtils::ToVPFromUnitValue(
                      max_width_str, screen_size[0], ctx->DevicePixelRatio()) *
                  scale_density;
    }
  }
  ParagraphStyleHarmony paragraph_style;
  if (info.Contains(kMaxLine)) {
    int32_t max_line = info.GetValueForKey(kMaxLine)->Number();
    if (max_line > 0) {
      paragraph_style.SetTextMaxLines(max_line);
    }
  }
  std::shared_ptr<FontFaceManager> font_face_manager =
      ctx->GetFontFaceManager();
  std::shared_ptr<FontCollectionHarmony> font_collection = nullptr;
  if (font_face_manager != nullptr) {
    font_collection = font_face_manager->GetFontCollection();
  }
  if (font_collection == nullptr) {
    font_collection = std::make_shared<FontCollectionHarmony>();
  }
  std::unique_ptr<ParagraphBuilderHarmony> builder =
      std::make_unique<ParagraphBuilderHarmony>(&paragraph_style,
                                                font_collection.get());
  TextStyleHarmony text_style;
  text_style.SetFontSize(font_size);
  if (info.Contains(kFontFamily)) {
    const std::string& font_family = info.GetValueForKey(kFontFamily)->str();
    if (!font_family.empty()) {
      text_style.SetFontFamiliesToStyle(font_family);
    }
  }

  builder->PushTextStyle(text_style);
  builder->AddText(content.c_str());
  builder->PopTextStyle();
  auto paragraph = builder->CreateParagraph(font_collection, 0);
  paragraph->Layout(max_width);
  width = paragraph->GetLongestLine() / scale_density;

  int line_count = paragraph->GetLineCount();
  if (line_count > 1) {
    auto u16content = base::U8StringToU16(content);
    auto line_str_array = lepus::CArray::Create();
    auto u16content_len = u16content.length();

    for (int i = 0; i < line_count; ++i) {
      auto line = paragraph->GetLineMetrics(i);
      if (line.EndIndex() <= u16content_len) {
        auto start_index = line.StartIndex();
        auto end_index = line.EndIndex();
        auto line_str = base::U16StringToU8(
            std::u16string_view(u16content)
                .substr(start_index, end_index - start_index));
        line_str_array->emplace_back(line_str.data());
      }
    }
    result.SetProperty(BASE_STATIC_STRING(kContent),
                       lepus::Value(line_str_array));
  }

  result.SetProperty(BASE_STATIC_STRING(kWidth), lepus::Value(width));
  return result;
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

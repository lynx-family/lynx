// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/text_fragment_behavior.h"

#include <chrono>
#include <utility>

#include "base/include/log/logging.h"
#include "core/event/custom_event.h"
#include "core/event/event_dispatcher.h"
#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fragment/fragment.h"

namespace lynx::tasm {

// Static string declarations for layout event keys
BASE_STATIC_STRING_DECL(kLineCount, "lineCount");
BASE_STATIC_STRING_DECL(kLines, "lines");
BASE_STATIC_STRING_DECL(kSize, "size");
BASE_STATIC_STRING_DECL(kStart, "start");
BASE_STATIC_STRING_DECL(kEnd, "end");
BASE_STATIC_STRING_DECL(kEllipsisCount, "ellipsisCount");

TextFragmentBehavior::TextFragmentBehavior(Fragment* fragment)
    : FragmentBehavior(fragment) {}

void TextFragmentBehavior::OnElementDestroying() {
  // Release platform resources while element is still accessible
  if (painting_context_ && text_bundle_ != 0) {
    painting_context_->DestroyTextBundle(fragment_->id());
    text_bundle_ = 0;
  }
}

void TextFragmentBehavior::CreatePlatformRenderer(
    const fml::RefPtr<PropBundle>& attributes) {
  if (painting_context_ && fragment_) {
    painting_context_->CreatePlatformRenderer(
        fragment_->id(), PlatformRendererType::kText, attributes);
  }
}

void TextFragmentBehavior::OnUpdateLayout(
    const LayoutInfoForDraw& layout_result) {
  if (painting_context_ && fragment_) {
    painting_context_->UpdateTextBundle(fragment_->id(), text_bundle_);
  }
  DispatchLayoutEvent(layout_result);
}

void TextFragmentBehavior::OnDraw(DisplayListBuilder& builder) {
  const auto& layout_info = fragment_->LayoutResult();
  builder.Begin(fragment_->id(),
                layout_info.layout_result.padding_[starlight::Direction::kLeft],
                layout_info.layout_result.padding_[starlight::Direction::kTop],
                layout_info.layout_result.size_.width_,
                layout_info.layout_result.size_.height_);
  builder.DrawText(fragment_->id());
  builder.End();
}

void TextFragmentBehavior::DispatchLayoutEvent(
    const LayoutInfoForDraw& layout_result) {
  if (!fragment_) {
    return;
  }

  Element* element = fragment_->element();
  if (!element) {
    return;
  }

  if (!element->HasEventListener("layout")) {
    return;
  }

  ElementManager* element_manager = element->element_manager();
  if (!element_manager) {
    return;
  }

  DCHECK(element->is_text());

  const float width = layout_result.GetContentBoxWidth();
  const float height = layout_result.GetContentBoxHeight();

  // Create layout event data with line information
  lepus::Value event_data(lepus::Dictionary::Create());
  // Get line layout info from TextElement (null-terminated array)
  const TextLineInfo* line_infos =
      static_cast<TextElement*>(element)->GetTextLineLayoutInfo();
  int line_count = static_cast<TextElement*>(element)->GetTextLineLayoutCount();

  if (line_count > 0) {
    // Add lineCount
    event_data.SetProperty(kLineCount, lepus::Value(line_count));

    // Add lines array
    lepus::Value lines_array(lepus::CArray::Create());
    for (int i = 0; i < line_count; i++) {
      const TextLineInfo& info = line_infos[i];
      lepus::Value line_info(lepus::Dictionary::Create());
      line_info.SetProperty(kStart, lepus::Value(info.start));
      line_info.SetProperty(kEnd, lepus::Value(info.end));
      line_info.SetProperty(kEllipsisCount, lepus::Value(info.ellipsis_count));
      lines_array.Array()->push_back(line_info);
    }
    event_data.SetProperty(kLines, lines_array);
  }

  // Add size information
  lepus::Value size_data(lepus::Dictionary::Create());
  size_data.SetProperty(BASE_STATIC_STRING(kWidth), lepus::Value(width));
  size_data.SetProperty(BASE_STATIC_STRING(kHeight), lepus::Value(height));
  event_data.SetProperty(kSize, size_data);

  int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  auto event = fml::MakeRefCounted<event::CustomEvent>("layout", event_data,
                                                       "detail", timestamp);
  event::EventDispatcher::DispatchEvent(*element, std::move(event));
}

}  // namespace lynx::tasm

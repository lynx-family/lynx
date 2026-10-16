// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_element_handle.h"

#include <cmath>
#include <limits>
#include <thread>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

constexpr char kOverlayTag[] = "overlay";

fml::RefPtr<Element> CreateContentElement(ElementManager* manager,
                                          ComposeElementKind kind,
                                          const base::String& tag) {
  switch (kind) {
    case ComposeElementKind::kView:
      return manager->CreateFiberView();
    case ComposeElementKind::kText:
      return manager->CreateFiberText(BASE_STATIC_STRING(kElementTextTag));
    case ComposeElementKind::kImage:
      return manager->CreateFiberImage(BASE_STATIC_STRING(kElementImageTag));
    case ComposeElementKind::kOverlay:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kOverlayTag));
    case ComposeElementKind::kCustom:
      return manager->CreateFiberElement(tag);
  }
  return nullptr;
}

}  // namespace

ComposeElementHandle::ComposeElementHandle(ElementManager* manager,
                                           ComposeElementKind kind,
                                           const base::String& tag)
    : content_element_(CreateContentElement(manager, kind, tag)),
      mount_root_(content_element_) {}

std::optional<LayoutResult> ComposeElementHandle::MeasureText(
    ElementManager* manager, double width, double width_mode, double height,
    double height_mode, const char** error) {
  auto fail = [error](const char* message) -> std::optional<LayoutResult> {
    *error = message;
    return std::nullopt;
  };
  if (!manager || content_element_->element_manager() != manager) {
    return fail("handle belongs to a different page");
  }
  const auto owner_thread = manager->GetCurrentEngineThreadId();
  if (!owner_thread || *owner_thread != std::this_thread::get_id()) {
    return fail("measurement requires the engine owner thread");
  }
  if (!manager->root()) {
    return fail("measurement requires an initialized page");
  }
  if (!content_element_->EnableLayoutInElementMode()) {
    return fail("measurement requires layout-in-element mode");
  }
  auto valid_size = [](double value) {
    return std::isfinite(value) && value >= 0 &&
           value <= std::numeric_limits<float>::max();
  };
  auto valid_mode = [](double value) {
    // SLMeasureMode: Indefinite=0, Definite=1, AtMost=2.
    return std::isfinite(value) && std::trunc(value) == value && value >= 0 &&
           value <= 2;
  };
  if (!valid_size(width) || !valid_size(height) || !valid_mode(width_mode) ||
      !valid_mode(height_mode)) {
    return fail("constraints must be finite nonnegative sizes and SL modes");
  }
  auto* content = content_element_.get();
  if (!content->is_text() || content->is_inline_element() ||
      content->parent() || content->render_parent() || content->IsAttached() ||
      content->attached_to_layout_parent() || mount_root_.get() != content ||
      !content->children().empty() || !content->logical_children().empty()) {
    return fail("measurement requires a detached childless Text handle");
  }
  auto* text = static_cast<TextElement*>(content);
  if (text->has_inline_child() || text->need_layout_children()) {
    return fail("inline text measurement is unsupported");
  }
  struct Cleanup {
    ElementManager* manager;
    TextElement* text;
    ~Cleanup() { manager->DestroyText(text); }
  } cleanup{manager, text};
  text->PrepareForDetachedMeasurement();
  text->DispatchLayoutBefore();
  const auto result = manager->MeasureDetached(
      text, static_cast<float>(width), static_cast<int32_t>(width_mode),
      static_cast<float>(height), static_cast<int32_t>(height_mode));
  if (!result) {
    return fail("native text measurement failed");
  }
  if (!std::isfinite(result->width_) || !std::isfinite(result->height_) ||
      !std::isfinite(result->baseline_) || result->width_ < 0 ||
      result->height_ < 0) {
    return fail("native text measurement returned invalid metrics");
  }
  return result;
}

}  // namespace tasm
}  // namespace lynx

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_element_handle.h"

#include <cmath>
#include <limits>
#include <thread>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

constexpr char kOverlayTag[] = "overlay";
constexpr char kSvgTag[] = "svg";
constexpr char kAnimaXTag[] = "animax-view";

fml::RefPtr<Element> CreateContentElement(ElementManager* manager,
                                          ComposeElementKind kind,
                                          const base::String& tag,
                                          TemplateAssembler* tasm) {
  switch (kind) {
    case ComposeElementKind::kView:
      return manager->CreateFiberView();
    case ComposeElementKind::kText:
      return manager->CreateFiberText(BASE_STATIC_STRING(kElementTextTag));
    case ComposeElementKind::kImage:
      return manager->CreateFiberImage(BASE_STATIC_STRING(kElementImageTag));
    case ComposeElementKind::kList:
      return manager->CreateFiberList(tasm, BASE_STATIC_STRING(kElementListTag),
                                      {}, {}, {});
    case ComposeElementKind::kSvg:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kSvgTag));
    case ComposeElementKind::kOverlay:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kOverlayTag));
    case ComposeElementKind::kAnimaX:
      return manager->CreateFiberElement(BASE_STATIC_STRING(kAnimaXTag));
    case ComposeElementKind::kCustom:
      return manager->CreateFiberElement(tag);
  }
  return nullptr;
}

}  // namespace

ComposeElementHandle::ComposeElementHandle(ElementManager* manager,
                                           ComposeElementKind kind,
                                           const base::String& tag,
                                           TemplateAssembler* tasm)
    : content_element_(CreateContentElement(manager, kind, tag, tasm)),
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
  if (!owner_thread || !(*owner_thread == std::this_thread::get_id())) {
    return fail("measurement requires the engine owner thread");
  }
  if (!manager->root()) {
    return fail("measurement requires an initialized page");
  }
  if (!content_element_->EnableLayoutInElementMode()) {
    return fail("measurement requires layout-in-element mode");
  }
  auto valid_size = [](double value) {
    // Finite bounds also reject NaN and infinities.
    return value >= 0 && value <= std::numeric_limits<float>::max();
  };
  auto valid_mode = [](double value) {
    // SLMeasureMode: Indefinite=0, Definite=1, AtMost=2.
    return value >= 0 && value <= 2 && std::trunc(value) == value;
  };
  if (!valid_size(width) || !valid_size(height) || !valid_mode(width_mode) ||
      !valid_mode(height_mode)) {
    return fail("constraints must be finite nonnegative sizes and SL modes");
  }
  const auto measure_width_mode = static_cast<SLMeasureMode>(width_mode);
  const auto measure_height_mode = static_cast<SLMeasureMode>(height_mode);
  // The JS contract uses CSS px; platform text layout uses layout units.
  const double layout_units_per_css_px =
      manager->GetLynxEnvConfig().LayoutsUnitPerPx();
  if (!std::isfinite(layout_units_per_css_px) || layout_units_per_css_px <= 0) {
    return fail("measurement requires a valid CSS pixel scale");
  }
  const double native_width = width * layout_units_per_css_px;
  const double native_height = height * layout_units_per_css_px;
  if (!valid_size(native_width) || !valid_size(native_height)) {
    return fail("constraints exceed the native layout unit range");
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
    ~Cleanup() {
      manager->DestroyDetachedText(text);
      // Native text state was released. Rebuild it and remeasure on the next
      // mount, even if preparation consumed all attribute updates. MarkDirty
      // clears the local layout cache without requesting a page layout.
      if (auto* node = text->slnode()) {
        node->MarkDirty();
      }
    }
  } cleanup{manager, text};
  text->PrepareForDetachedMeasurement();
  text->DispatchLayoutBefore();
  const auto result = manager->MeasureDetached(
      text, static_cast<float>(native_width), measure_width_mode,
      static_cast<float>(native_height), measure_height_mode);
  if (!result) {
    return fail("native text measurement failed");
  }
  const double css_width = result->width_ / layout_units_per_css_px;
  const double css_height = result->height_ / layout_units_per_css_px;
  const double css_baseline = result->baseline_ / layout_units_per_css_px;
  if (!valid_size(css_width) || !valid_size(css_height) ||
      !valid_size(std::abs(css_baseline))) {
    return fail("native text measurement returned invalid metrics");
  }
  // As in mounted layout, definite constraints override the backend's natural
  // dimensions. Use the original CSS sizes to avoid a layout-unit round trip.
  return LayoutResult{
      static_cast<float>(
          measure_width_mode == SLMeasureModeDefinite ? width : css_width),
      static_cast<float>(
          measure_height_mode == SLMeasureModeDefinite ? height : css_height),
      static_cast<float>(css_baseline)};
}

}  // namespace tasm
}  // namespace lynx

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_element_handle.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/custom_layout.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

constexpr char kOverlayTag[] = "overlay";
constexpr char kSvgTag[] = "svg";

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

ComposeElementHandle::~ComposeElementHandle() {
  if (custom_layout_) {
    ClearCustomLayout();
    if (!content_element_->will_destroy()) content_element_->RequestLayout();
  }
}

void ComposeElementHandle::ClearCustomLayout() {
  if (!custom_layout_) return;
  content_element_->SetMeasureFunc(nullptr, nullptr);
  content_element_->SetAlignmentFunc(nullptr, nullptr);
  custom_layout_->Detach();
  custom_layout_.reset();
}

void ComposeElementHandle::visitor(void* rt, void* func, uint64_t trace_tool) {
  if (custom_layout_) custom_layout_->visitor(rt, func, trace_tool);
  content_element_->visitor(rt, func, trace_tool);
}

bool ComposeElementHandle::SetCustomLayout(
    std::shared_ptr<CustomLayout> layout) {
  auto& owner = *content_element_;
  if (!owner.EnableLayoutInElementMode() || owner.will_destroy() ||
      (layout && !owner.GetEnableZIndex()) || owner.GetTag() != "view" ||
      owner.IsShadowNodeCustom() || (layout && &layout->owner() != &owner) ||
      (custom_layout_ && custom_layout_->IsExecuting()))
    return false;
  owner.UpdateLayoutNodeByBundle();
  if (!custom_layout_ && (owner.slnode()->GetSLMeasureFunc() ||
                          owner.slnode()->GetSLAlignmentFunc()))
    return false;
  ClearCustomLayout();
  custom_layout_ = std::move(layout);
  if (custom_layout_) {
    owner.SetMeasureFunc(
        custom_layout_.get(),
        [](void* context, const starlight::Constraints& constraints,
           bool final_measure) {
          auto& width = constraints[starlight::kHorizontal];
          auto& height = constraints[starlight::kVertical];
          auto size = static_cast<CustomLayout*>(context)->Measure(
              IsSLIndefiniteMode(width.Mode()) ? 0 : width.Size(), width.Mode(),
              IsSLIndefiniteMode(height.Mode()) ? 0 : height.Size(),
              height.Mode(), final_measure);
          return FloatSize(size.width_, size.height_, size.baseline_);
        });
    owner.SetAlignmentFunc(custom_layout_.get(), [](void* context) {
      static_cast<CustomLayout*>(context)->Alignment();
    });
  }
  owner.RequestLayout();
  return true;
}

}  // namespace tasm
}  // namespace lynx

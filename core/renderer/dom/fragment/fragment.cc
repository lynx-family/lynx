// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment.h"

#include <memory>
#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx {
namespace tasm {

Fragment::Fragment(Element* element) : ElementContainer(element) {}

void Fragment::CreateLayerIfNeeded() {
  // TODO(zhongyr): abstract one behavior for layerize.
  if ((!element()->TendToFlatten() && !has_platform_renderer_ && behavior_) ||
      element()->is_page()) {
    behavior_->CreatePlatformRenderer();
    has_platform_renderer_ = true;
  }
}

void Fragment::CreatePaintingNode(
    bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  MarkNeedRedraw();
  element()->SetupFragmentBehavior(this);
  CreateLayerIfNeeded();
}

void Fragment::UpdatePaintingNode(
    bool tend_to_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  MarkNeedRedraw();
  // TODO(zhongyr): handle update or tend to flatten here.
}

// TODO(zhongyr): Finish Update attributes Fragment tree related operations.
// Should create displayList and update it to platform renderer.

void Fragment::UpdateLayout(
    LayoutResultForRendering layout_result_for_rendering) {
  MarkNeedRedraw();
  layout_result_for_rendering_ = std::move(layout_result_for_rendering);
}

void Fragment::SetBehavior(std::unique_ptr<FragmentBehavior> behavior) {
  behavior_ = std::move(behavior);
}

void Fragment::OnDraw(DisplayListBuilder& display_list_builder) {
  display_list_builder.Begin(layout_result_for_rendering_.offset_.X(),
                             layout_result_for_rendering_.offset_.Y(),
                             layout_result_for_rendering_.size_.width_,
                             layout_result_for_rendering_.size_.height_);

  if (behavior_) {
    behavior_->OnDraw(display_list_builder);
  }

  for (const auto& child : children_) {
    child->Draw(display_list_builder);
  }

  display_list_builder.End();
}

void Fragment::Draw() {
  // XXX: Maybe this part could run parallely with parent displayList
  // generation. The shared totally different context.

  //  Collect own displayList.
  DisplayListBuilder builder;

  OnDraw(builder);

  painting_context()->impl()->CastToNativeCtx()->UpdateDisplayList(
      id(), builder.Build());
}

void Fragment::Draw(DisplayListBuilder& display_list_builder) {
  if (has_platform_renderer_) {
    display_list_builder.DrawView(id());
    // The view got its own display list.
    Draw();
    return;
  }

  OnDraw(display_list_builder);
}

void Fragment::MarkNeedRedraw() { need_redraw_ = true; }

void Fragment::InsertElementContainerAccordingToElement(Element* child,
                                                        Element* ref) {
  AddChildBefore(
      static_cast<Fragment*>(child->element_container()),
      static_cast<Fragment*>(ref ? ref->element_container() : nullptr));
}
void Fragment::AddChildBefore(Fragment* child, Fragment* sibling) {
  if (!sibling) {
    children_.emplace_back(child);
    return;
  }

  if (auto it = std::find(children_.begin(), children_.end(), sibling);
      it != children_.end()) {
    children_.insert(it, child);
  }
}

}  // namespace tasm
}  // namespace lynx

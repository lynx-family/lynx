// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

#include <memory>

#include "core/renderer/dom/base_element_container.h"
#include "core/renderer/dom/fragment/box_model_recorder.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/layout_info.h"

namespace lynx {
namespace starlight {
class ComputedCSSStyle;
}
namespace tasm {

using starlight::LayoutResultForRendering;

// Combines layout results and rendering styles to generate display content
// via DisplayList. Owned by an element; lifetime must not exceed that element.
class Fragment : public BaseElementContainer {
 public:
  explicit Fragment(Element* element);

  ~Fragment() override = default;

  // Returns the parent of this fragment in the fragment tree.
  Fragment* fragment_parent() const;

  // Returns the fragment that is the parent of this fragment in the element
  // tree.
  Fragment* fragment_from_element_parent() const {
    return fragment_from_element_parent_;
  }

  void set_fragment_from_element_parent(
      Fragment* fragment_from_element_parent) {
    fragment_from_element_parent_ = fragment_from_element_parent;
  }

  bool HasUIPrimitive() const override;

  void InsertElementContainerAccordingToElement(Element* child,
                                                Element* ref) override;

  void RemoveElementContainerAccordingToElement(Element* child,
                                                bool destroy) override;
  void Destroy() override {
    if (behavior_) {
      behavior_->Destroy();
    }
    if (has_platform_renderer_) {
      painting_context()->DestroyPaintingNode(
          0 /* will be ignored, directly destroy it */, id(),
          -1 /* will be ignored when remove*/);
    }
  };

  void UpdateLayout(float left, float top,
                    bool transition_view = false) override;
  void UpdateLayoutWithoutChange() override{};

  void TransitionToNativeView(fml::RefPtr<PropBundle> prop_bundle) override{};
  void StyleChanged() override;
  void UpdateZIndexList() override;

  void CreatePaintingNode(
      bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) override;
  void UpdatePaintingNode(
      bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override;

  void InsertListItemPaintingNode(int32_t child_id) override;
  void RemoveListItemPaintingNode(int32_t child_id) override;
  void UpdateContentOffsetForListContainer(float content_size, float delta_x,
                                           float delta_y,
                                           bool is_init_scroll_offset,
                                           bool from_layout) override;

  void CreateLayerIfNeeded(const fml::RefPtr<PropBundle>& init_data);
  void HandleAttributes(const fml::RefPtr<PropBundle>& painting_data) const;

  void UpdateLayout(LayoutResultForRendering layout_result_for_rendering);

  void SetBehavior(std::unique_ptr<FragmentBehavior> behavior);

  // Called when the element is being destroyed. Notifies the behavior to
  // release platform resources while they are still accessible.
  void OnElementDestroying();

  void Draw();

  void Draw(DisplayListBuilder& display_list_builder);

  void OnDraw(DisplayListBuilder& display_list_builder);

  void SetEventProp(PlatformEventPropName name, const lepus::Value& value);

  void ClearEventProps();

  void AddEventName(PlatformEventName name);

  void ClearEventNames();

  const PlatformEventPropMap& EventProps() const { return event_props_; }

  const base::Vector<PlatformEventName>& EventNames() const {
    return event_names_;
  }

  void DrawChildren(DisplayListBuilder& display_list_builder);

  void AddChildBefore(Fragment* child, Fragment* sibling);

  void RemoveSelf();

  void RemoveChild(Fragment* child);

  void UpdatePlatformExtraBundle(PlatformExtraBundle* bundle) override;

  bool IsReliableSibling() const;

  const auto& LayoutResult() const { return layout_info_; }

  int32_t DefineBorderBox(DisplayListBuilder& display_list_builder);
  int32_t DefinePaddingBox(DisplayListBuilder& display_list_builder);
  int32_t DefineContentBox(DisplayListBuilder& display_list_builder);

  void SetTextBundle(intptr_t bundle);

 protected:
  static const int32_t kDefaultDrawNodeCapacity;

  bool is_fragment() const override { return true; }

 private:
  void CheckRootIfNeedClipBounds(DisplayListBuilder& display_list_builder);
  void UpdateBorderRadiusAccordingToLayoutInfo();
  void UpdateRenderOffsetRecursively(float left, float top, Fragment* root);

  void DrawBorder(DisplayListBuilder& display_list_builder);
  void DrawClip(DisplayListBuilder& display_list_builder);

  void DrawBackground(DisplayListBuilder& display_list_builder);
  void DrawTransform(DisplayListBuilder& display_list_builder);
  void DrawOpacity(DisplayListBuilder& display_list_builder);

  // Performs a full redraw of this fragment including background, border,
  // children, etc. Called by OnDraw when NeedRedraw() is true.
  void DrawFull(DisplayListBuilder& display_list_builder);

  void ReinsertDescendantsToCorrectParent();

  void RemoveDescendantsFromCurrentParent();

  void MoveDirectStackingChildren(Fragment* parent, Fragment* child);

  void MarkHasExposureEventIfNeeded() const;

  void ReconstructEventTargetTreeForExposure() const;

  void DispatchUpdateDisplayList();

  bool has_platform_renderer_{false};

  // If the fragment has positon fixed or z-index != 0, store the fragment from
  // element parent using this pointer. Which means if the
  // fragment_from_element_parent_ is not null, the fragment has position fixed
  // or z-index != 0.
  Fragment* fragment_from_element_parent_{nullptr};

  base::MoveOnlyClosure<bool> should_create_layer_;

  // TODO(zhongyr): children management methods.
  base::InlineVector<Fragment*, kChildrenInlineVectorSize> children_;

  // Store the children fragment with z-index, which's parent may not equal to
  // this but the corresponding element's parent is current fragment's element.
  base::InlineLinearFlatSet<Fragment*, kChildrenInlineVectorSize> z_children_;

  // Store the children fragment with position fixed, which's parent may not
  // equal to this but the corresponding element's parent is current fragment's
  // element.
  base::InlineLinearFlatSet<Fragment*, kChildrenInlineVectorSize>
      fixed_children_;

  LayoutInfoForDraw layout_info_;

  BoxModelRecorder box_recorder_;

  std::unique_ptr<FragmentBehavior> behavior_;

  PlatformEventPropMap event_props_;
  base::Vector<PlatformEventName> event_names_;

  float render_offset_[2] = {0, 0};

  int32_t draw_node_capacity_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

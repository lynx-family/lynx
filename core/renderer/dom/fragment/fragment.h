// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

#include <memory>

#include "base/include/geometry/point.h"
#include "base/include/value/base_string.h"
#include "core/renderer/dom/base_element_container.h"
#include "core/renderer/dom/fragment/box_model_recorder.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/layout_info.h"

namespace lynx {
namespace starlight {
class ComputedCSSStyle;
}
namespace tasm {

using starlight::LayoutResultForRendering;

class Fragment;

// The only resolved geometry consumed by fragment painting. |offset_to_parent|
// is expressed in the coordinate space of |parent|. For flattened fragments
// the parent is the direct fragment parent; for platform-backed fragments it
// is the nearest platform-backed ancestor.
//
// A platform-backed fragment is represented by both a native child view and
// its own root display list. Some platforms position that child using
//   paint_offset + platform_embedding_offset
// and cancel |platform_embedding_offset| while dispatching the child canvas.
// These two values are derived here from |offset_to_parent| and the active
// StackingTree paint path; they are not a second recursively maintained offset
// state.
struct ResolvedStackingGeometry {
  Fragment* parent{nullptr};
  // Nearest platform-backed owner of this fragment's display-list content.
  // Restacking carries it down the tree instead of rediscovering it by
  // walking the Fragment parent chain for every changed descendant.
  Fragment* paint_root{nullptr};
  base::geometry::FloatPoint offset_to_parent{0.f, 0.f};
  base::geometry::FloatPoint paint_offset{0.f, 0.f};
  base::geometry::FloatPoint platform_embedding_offset{0.f, 0.f};
  bool valid{false};
};

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
  void UpdateLayoutWithoutChange() override;

  void InvalidateForRedraw() override;

  void TransitionToNativeView(fml::RefPtr<PropBundle> prop_bundle) override {}
  void StyleChanged() override;
  void UpdateZIndexList() override;

  void CreatePaintingNode(
      bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) override;
  void UpdatePaintingNode(
      bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override;

  void OnFirstScreen() override;
  void OnNodeReady() override;
  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override;

  bool CreateLayerIfNeeded(const fml::RefPtr<PropBundle>& init_data);
  void HandleAttributes(const fml::RefPtr<PropBundle>& painting_data) const;

  void UpdateLayout(LayoutResultForRendering layout_result_for_rendering);

  void SetBehavior(std::unique_ptr<FragmentBehavior> behavior);

  // Called when the element is being destroyed. Notifies the behavior to
  // release platform resources while they are still accessible.
  void OnElementDestroying();

  void Draw();

  void Draw(DisplayListBuilder& display_list_builder);

  // Resolves LayoutTree coordinates onto StackingTree/paint edges. This is a
  // distinct pipeline phase and is a no-op until an input edge is invalidated.
  void RestackIfNeeded();

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

  enum class PaintOrderGroup : uint8_t {
    kNegativeZ,
    kNormalFlow,
    kFixedZero,
    kPositiveZ,
  };
  using PaintOrderBucket = base::InlineVector<Fragment*, 2>;
  struct PaintOrderBuckets {
    PaintOrderBucket negative_z;
    PaintOrderBucket fixed_zero;
    PaintOrderBucket positive_z;
    bool negative_z_dirty{false};
    bool fixed_zero_dirty{false};
    bool positive_z_dirty{false};
  };

  static PaintOrderGroup PaintGroupFor(const Fragment* child);
  static bool ZPaintOrderLess(const Fragment* left, const Fragment* right);
  static bool DocumentOrderLess(const Fragment* left, const Fragment* right);
  bool ShouldBypassPaintOrderBuckets() const;
  void AppendToPaintOrderBucket(Fragment* child);
  void InsertIntoPaintOrderBucket(Fragment* child);
  void RemoveFromPaintOrderBucket(Fragment* child);
  size_t PaintOrderIndex(const Fragment* child) const;

  void AddChildBefore(Fragment* child, Fragment* sibling);

  void RemoveSelf();

  void RemoveChild(Fragment* child);

  void UpdatePlatformExtraBundle(PlatformExtraBundle* bundle) override;

  bool IsReliableSibling() const;

  const auto& LayoutResult() const { return layout_info_; }

  const ResolvedStackingGeometry& stacking_geometry() const {
    return stacking_geometry_;
  }

  bool ShouldSyncLayoutOnlyToEventTarget() const;

  int32_t DefineBorderBox(DisplayListBuilder& display_list_builder);
  int32_t DefinePaddingBox(DisplayListBuilder& display_list_builder);
  int32_t DefineContentBox(DisplayListBuilder& display_list_builder);

  void SetTextBundle(intptr_t bundle);

 protected:
  static const int32_t kDefaultDrawNodeCapacity;

  bool is_fragment() const override { return true; }

 private:
  friend class Element;

  void CheckRootIfNeedClipBounds(DisplayListBuilder& display_list_builder);
  void UpdateBorderRadiusAccordingToLayoutInfo();
  void UpdateLayoutRecursively(
      Fragment* draw_root, uint64_t restacking_generation = 0,
      base::geometry::FloatPoint active_paint_offset = {0.f, 0.f},
      Fragment* active_paint_root = nullptr);

  void InvalidateRestacking();
  void InvalidateLayoutOffsetCache();
  Fragment* RestackingRoot();
  uint64_t PrepareRestacking();
  void CollectLayoutOffsetsToRoot(Element* current,
                                  base::geometry::FloatPoint parent_offset);
  bool CacheLayoutOffsetToRoot(base::geometry::FloatPoint parent_offset);
  bool RefreshLayoutOffsetSubtree();
  void FinishIncrementalLayoutOffsetUpdate(Fragment* restacking_root,
                                           bool collection_was_pending);
  void FinishLayoutOffsetCollection();
  bool ResolveStackingGeometry(
      base::geometry::FloatPoint active_paint_offset,
      Fragment* active_paint_root, bool flush_node_ready,
      base::geometry::FloatPoint* child_active_paint_offset,
      Fragment** child_active_paint_root);
  void ResolveStackingGeometryRecursively(
      base::geometry::FloatPoint active_paint_offset,
      Fragment* active_paint_root, bool flush_node_ready);
  Fragment* ResolveStackingGeometryParent() const;
  Fragment* ResolveEnclosingStackingContextParent() const;
  static void MarkResolvedPaintRootDirty(Fragment* paint_root);
  void ReparentStackingNode(Fragment* target_parent, Fragment* sibling);

  void DrawBorder(DisplayListBuilder& display_list_builder);
  void DrawClip(DisplayListBuilder& display_list_builder);

  void DrawBackground(DisplayListBuilder& display_list_builder);
  void DrawBoxShadow(DisplayListBuilder& display_list_builder);
  void DrawTransform(DisplayListBuilder& display_list_builder);
  void DrawOpacity(DisplayListBuilder& display_list_builder);
  void DrawFilter(DisplayListBuilder& display_list_builder);

  // Performs a full redraw of this fragment including background, border,
  // children, etc. Called by OnDraw when NeedRedraw() is true.
  void DrawFull(DisplayListBuilder& display_list_builder);

  bool ShouldSyncNativePlatformRenderer() const;
  void MarkNodeReadyIfNeeded();
  void FlushPendingNodeReadyIfNeeded();

  void ReinsertDescendantsToCorrectParent();

  void RemoveDescendantsFromCurrentParent();

  void MoveDirectStackingChildren(Fragment* parent, Fragment* child);

  void MarkHasExposureEventIfNeeded() const;

  void ReconstructEventTargetTreeForExposure() const;

  void DispatchUpdateDisplayList();

  struct BackgroundImageResource {
    base::String url;
    fml::RefPtr<PaintImage> image;
  };

  fml::RefPtr<PaintImage> GetOrCreateBackgroundImage(size_t image_index,
                                                     const base::String& url,
                                                     float image_width,
                                                     float image_height);
  void ClearBackgroundImage(size_t image_index);

  bool has_platform_renderer_{false};
  bool pending_node_ready_{false};

  // If the fragment has positon fixed or z-index != 0, store the fragment from
  // element parent using this pointer. Which means if the
  // fragment_from_element_parent_ is not null, the fragment has position fixed
  // or z-index != 0.
  Fragment* fragment_from_element_parent_{nullptr};

  base::MoveOnlyClosure<bool> should_create_layer_;

  // TODO(zhongyr): children management methods.
  base::InlineVector<Fragment*, kChildrenInlineVectorSize> children_;

  // Paint order is independent from the structural FragmentTree order in
  // children_. Normal-flow children are read directly from children_; only
  // groups that require z/fixed ordering are cached here.
  std::unique_ptr<PaintOrderBuckets> paint_order_buckets_;

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
  base::Vector<BackgroundImageResource> background_image_resources_;
  bool event_bundle_dirty_{false};

  // Resolver input. This belongs to the LayoutTree coordinate space and is
  // never read by drawing code.
  base::geometry::FloatPoint layout_offset_to_root_{0.f, 0.f};
  int32_t layout_parent_id_for_cached_offset_{-1};
  bool layout_offset_valid_{false};

  ResolvedStackingGeometry stacking_geometry_;
  bool layout_geometry_initialized_{false};
  bool needs_restacking_{true};
  // The regular Element layout-info traversal maintains persistent root
  // offsets. A full LayoutTree fallback is only needed when a direct Fragment
  // update bypasses that traversal.
  bool needs_layout_offset_collection_{true};
  bool updating_layout_offset_cache_{false};
  uint64_t restacking_generation_{0};

  int32_t draw_node_capacity_{0};
};

// Computes the outset-adjusted border radius for box-shadow spread
// interpolation per W3C CSS Backgrounds and Borders Module Level 3.
float ComputeOutsetAdjustedRadius(float radius, float spread, float coverage);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

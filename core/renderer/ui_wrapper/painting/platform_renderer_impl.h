// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_IMPL_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_IMPL_H_

#include <cstddef>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer.h"
#include "core/renderer/utils/base/base_def.h"

constexpr const static int32_t kRootId = 10;

namespace lynx::tasm {

class DisplayList;
class PropBundle;

// Platform-agnostic base implementation that provides common functionality
// for all platform-specific renderers. Platform-specific renderers should
// inherit from this class to share common logic.
class PlatformRendererImpl : public PlatformRenderer {
  using ChildVecT = base::InlineVector<fml::RefPtr<PlatformRenderer>,
                                       kChildrenInlineVectorSize>;

 public:
  explicit PlatformRendererImpl(int id, PlatformRendererType type,
                                const base::String& tag);

  ~PlatformRendererImpl() override;

  // PlatformRenderer interface.
  // Content
  void UpdateDisplayList(DisplayList display_list) override;

  // for layer only.
  void UpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                        bool tends_to_flatten) override;
  const DisplayList& GetDisplayList() const { return display_list_; }
  const SubtreeProperty* GetTransform() const { return transform_.get(); }
  void UpdateLayoutMetrics(float left, float top, float width, float height,
                           const float* paddings, const float* margins,
                           const float* borders);
  bool HasLayoutMetrics() const { return has_layout_metrics_; }
  const float* GetLayoutPaddings() const { return layout_paddings_; }
  const float* GetLayoutMargins() const { return layout_margins_; }
  const float* GetLayoutBorders() const { return layout_borders_; }

  void RemoveFromParent() override;
  void AddChild(fml::RefPtr<PlatformRenderer> child, int index = -1) override;

  const ChildVecT& Children() const override { return children_; }

  int GetId() const override { return id_; }
  const base::String& GetTagName() const { return tag_name_; }
  bool IsOverlay() const { return is_overlay_; }
  PlatformRendererType GetPlatformRendererType() const { return type_; }

  base::String GetExtendedRendererTagName() const override;

 protected:
  void ReleaseSelf() const override;

  bool IsPlatformExtendedRenderer() const {
    return is_platform_extended_renderer_;
  }
  // Marks a renderer that is created for the direct child of a compatible
  // component. These renderers are backed by UIOwner even when their type would
  // normally use the plain renderer-host path.
  void SetDirectChildOfCompatibleComponent(bool value) {
    is_direct_child_of_compatible_component_ = value;
  }

  // Records the parent in the element tree. The fragment tree may reparent
  // nodes for z-index/fixed handling, but UIOwner relations should only follow
  // the original element parent.
  void SetFragmentParentId(int32_t fragment_parent_id) {
    fragment_parent_id_ = fragment_parent_id;
  }

 private:
  void UpdateSubtreeProperty(const DisplayList& display_list);

  // Whether the parent-child edge should also be mirrored through UIOwner.
  // Only direct element-parent relations between UIOwner-backed renderers
  // should update UIOwner; other renderer-host relations are managed as native
  // views.
  bool ShouldUpdateUIOwnerForChild(const PlatformRendererImpl& child) const;

 protected:
  // Platform-specific operations to be implemented by derived classes
  virtual void OnUpdateDisplayList(DisplayList display_list) = 0;
  virtual void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                  bool tends_to_flatten) = 0;
  // `should_update_ui_owner` is true when the platform implementation should
  // update the UIOwner tree instead of directly mutating renderer-host views.
  virtual void OnAddChild(PlatformRenderer* child, int index,
                          bool should_update_ui_owner) = 0;
  virtual void OnRemoveFromParent(bool should_update_ui_owner) = 0;
  virtual void OnUpdateSubtreeProperties(
      const DisplayList& subtree_properties) = 0;

  // Get the parent renderer
  PlatformRendererImpl* GetParent() const { return parent_; }

  int id_;
  PlatformRendererType type_;
  base::String tag_name_;

  SubtreeProperty opacity_;
  base::auto_create_optional<SubtreeProperty> transform_;

  PlatformRendererImpl* parent_ = nullptr;

  DisplayList display_list_;
  ChildVecT children_;
  float layout_frame_[4] = {0.f, 0.f, 0.f, 0.f};
  float layout_paddings_[4] = {0.f, 0.f, 0.f, 0.f};
  float layout_margins_[4] = {0.f, 0.f, 0.f, 0.f};
  float layout_borders_[4] = {0.f, 0.f, 0.f, 0.f};
  bool has_layout_metrics_ = false;
  bool is_platform_extended_renderer_ = false;
  bool is_direct_child_of_compatible_component_ = false;
  // True when the current parent edge is mirrored through UIOwner.
  bool is_ui_owner_child_ = false;
  int32_t fragment_parent_id_ = -1;
  bool is_overlay_ = false;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_IMPL_H_

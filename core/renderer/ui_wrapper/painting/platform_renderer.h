// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_H_

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/vector.h"
#include "core/public/platform_renderer_type.h"
#include "core/public/prop_bundle.h"
#include "core/public/pub_value.h"
#include "core/renderer/utils/base/base_def.h"
#include "gfx/geometry/matrix44.h"

namespace lynx::lepus {
class Value;
}

namespace lynx::tasm {

class DisplayList;
class PropBundle;
class PlatformExtraBundle;

// Published and consumed on the platform thread. Position is in the parent
// content coordinates, excluding CSS transforms and parent scrolling.
struct PlatformRendererState {
  std::optional<std::array<float, 2>> layout_position;
  std::optional<std::array<float, 2>> scroll_offset;
  std::array<float, 2> sticky_offset = {0.f, 0.f};
  // Unknown membership preserves behavior for hosts that do not publish state.
  std::optional<bool> is_on_tree;
  // Back-to-front drawing order among siblings, published by the native parent.
  std::optional<int32_t> sibling_order;
  // Complete local-to-parent View transform, replacing the CSS transform when
  // a native component also transforms its child host.
  struct ViewTransform {
    gfx::Matrix44 matrix;
    std::array<float, 2> size;
  };
  std::unique_ptr<ViewTransform> view_transform;
};

// Initialization metadata consumed only by the C++ PlatformRenderer creation
// path. Platform-specific props should still be passed through PropBundle.
struct PlatformRendererInitConfig {
  int32_t fragment_parent_id = -1;
  bool is_direct_child_of_compatible_component = false;
};

// Abstract base class for platform-specific UI rendering operations.
// Provides a common interface for cross-platform UI element management.
class PlatformRenderer : public fml::RefCountedThreadSafeStorage,
                         public fml::EnableWeakFromThis<PlatformRenderer> {
 public:
  ~PlatformRenderer() override = default;
  // Update the display list for this renderer
  virtual void UpdateDisplayList(DisplayList display_list) = 0;

  // Update attribute bundle for this renderer. Default implementation is
  // provided in PlatformRendererImpl.
  virtual void UpdateAttributes(const fml::RefPtr<PropBundle>& attributes) = 0;
  // Add a child renderer
  virtual void AddChild(fml::RefPtr<PlatformRenderer> child,
                        int index = -1) = 0;

  virtual void OnRebuildSubRenderers() {}

  // Remove this renderer from its parent
  virtual void RemoveFromParent() = 0;

  // Get the unique identifier for this renderer
  virtual int GetId() const = 0;

  virtual const base::InlineVector<fml::RefPtr<PlatformRenderer>,
                                   kChildrenInlineVectorSize>&
  Children() const = 0;

  virtual base::String GetExtendedRendererTagName() const = 0;

  // Unpublished fields preserve the existing layout and platform queries.
  const PlatformRendererState* GetPlatformState() const {
    return &platform_state_;
  }
  void UpdateLayoutPosition(float x, float y) {
    platform_state_.layout_position = {x, y};
  }
  void UpdateScrollOffset(float x, float y) {
    platform_state_.scroll_offset = {x, y};
  }
  void UpdateViewTransform(const float* matrix, float width, float height) {
    if (!matrix) {
      platform_state_.view_transform.reset();
      return;
    }
    if (!platform_state_.view_transform) {
      platform_state_.view_transform =
          std::make_unique<PlatformRendererState::ViewTransform>();
    }
    // Embed the projected 2D homography, so inversion preserves perspective.
    platform_state_.view_transform->matrix = gfx::Matrix44(
        matrix[0], matrix[1], 0, matrix[2], matrix[3], matrix[4], 0, matrix[5],
        0, 0, 1, 0, matrix[6], matrix[7], 0, matrix[8]);
    platform_state_.view_transform->size = {width, height};
  }
  void UpdateStickyOffset(float x, float y, bool sticky) {
    platform_state_.sticky_offset =
        sticky ? std::array<float, 2>{x, y} : std::array<float, 2>{0.f, 0.f};
  }
  void UpdateSiblingOrder(int32_t order) {
    platform_state_.sibling_order = order;
  }
  void UpdateIsOnTree(bool is_on_tree) {
    if (!is_on_tree) {
      platform_state_ = {};
    }
    platform_state_.is_on_tree = is_on_tree;
  }
  // Logical removal resets geometry; the native parent owns membership and
  // order.
  void ClearPlatformState() {
    platform_state_.layout_position.reset();
    platform_state_.scroll_offset.reset();
    platform_state_.sticky_offset = {0.f, 0.f};
    platform_state_.view_transform.reset();
  }

  // Invoke a UI method on the platform renderer host. Return true only when the
  // renderer handled the method and will own the callback; return false to let
  // the caller fall back to the platform UI implementation.
  virtual bool InvokeUIMethod(
      const std::string& method, const lepus::Value& params,
      base::MoveOnlyClosure<void, int32_t, const pub::Value&>& callback) {
    return false;
  }

  void ReleaseSelf() const override = 0;

 private:
  PlatformRendererState platform_state_;
};

// Factory interface for creating platform-specific renderers
class PlatformRendererFactory {
 public:
  virtual ~PlatformRendererFactory() = default;

  // Create a new platform renderer with the given ID
  virtual fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig()) = 0;

  virtual fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig()) = 0;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_H_

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

  ~PlatformRendererImpl() override = default;

  // PlatformRenderer interface.
  // Content
  void UpdateDisplayList(DisplayList display_list) override;

  // for layer only.
  void UpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                        bool tends_to_flatten) override;
  const DisplayList& GetDisplayList() const { return display_list_; }

  void RemoveFromParent() override;
  void AddChild(fml::RefPtr<PlatformRenderer> child) override;

  const ChildVecT& Children() const override { return children_; }

  int GetId() const override { return id_; }

 protected:
  void ReleaseSelf() const override;

  bool IsPlatformExtendedRenderer() const {
    return is_platform_extended_renderer_;
  }

  PlatformRendererType GetPlatformRendererType() const { return type_; }

 private:
  void UpdateSubtreeProperty(const DisplayList& display_list);

 protected:
  // Platform-specific operations to be implemented by derived classes
  virtual void OnUpdateDisplayList(DisplayList display_list) = 0;
  virtual void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                  bool tends_to_flatten) = 0;
  virtual void OnAddChild(PlatformRenderer* child) = 0;
  virtual void OnRemoveFromParent() = 0;
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
  bool is_platform_extended_renderer_ = false;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_IMPL_H_

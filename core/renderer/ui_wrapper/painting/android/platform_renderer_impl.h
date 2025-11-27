// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_IMPL_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_IMPL_H_

#include <cstddef>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx::tasm {

class DisplayList;

// Platform-agnostic base implementation that provides common functionality
// for all platform-specific renderers. Platform-specific renderers should
// inherit from this class to share common logic.
class PlatformRendererImpl : public PlatformRenderer {
  using ChildVecT = base::InlineVector<fml::RefPtr<PlatformRenderer>,
                                       kChildrenInlineVectorSize>;

 public:
  explicit PlatformRendererImpl(int id) : id_(id) {}

  ~PlatformRendererImpl() override = default;

  // PlatformRenderer interface
  void UpdateDisplayList(DisplayList display_list) override;

  void RemoveFromParent() override;
  void AddChild(fml::RefPtr<PlatformRenderer> child) override;

  const ChildVecT& Children() const override { return children_; }

  int GetId() const override { return id_; }

 protected:
  void ReleaseSelf() const override;

 protected:
  // Platform-specific operations to be implemented by derived classes
  virtual void OnUpdateDisplayList(DisplayList display_list) = 0;
  virtual void OnAddChild(PlatformRenderer* child) = 0;
  virtual void OnRemoveFromParent() = 0;

  // Get the parent renderer
  PlatformRendererImpl* GetParent() const { return parent_; }

 private:
  int id_;
  PlatformRendererImpl* parent_ = nullptr;
  ChildVecT children_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_IMPL_H_

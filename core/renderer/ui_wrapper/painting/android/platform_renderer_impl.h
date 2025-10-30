// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_IMPL_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_IMPL_H_

#include <memory>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer.h"

namespace lynx::tasm {

class DisplayList;

// Platform-agnostic base implementation that provides common functionality
// for all platform-specific renderers. Platform-specific renderers should
// inherit from this class to share common logic.
class PlatformRendererImpl : public PlatformRenderer {
 public:
  explicit PlatformRendererImpl(int id)
      : id_(id), parent_(nullptr), valid_(true) {}

  ~PlatformRendererImpl() override = default;

  // PlatformRenderer interface
  void UpdateDisplayList(const DisplayList& display_list) override;
  void AddChild(std::unique_ptr<PlatformRenderer> child) override;
  void RemoveFromParent() override;
  int GetId() const override { return id_; }
  bool IsValid() const override { return valid_; }

 protected:
  void ReleaseSelf() const override;

 protected:
  // Platform-specific operations to be implemented by derived classes
  virtual void OnUpdateDisplayList(const DisplayList& display_list) = 0;
  virtual void OnAddChild(PlatformRenderer* child) = 0;
  virtual void OnRemoveFromParent() = 0;

  // Get the parent renderer
  PlatformRendererImpl* GetParent() const { return parent_; }

  // Get all child renderers
  const std::vector<std::unique_ptr<PlatformRenderer>>& GetChildren() const {
    return children_;
  }

  // Mark this renderer as invalid
  void Invalidate() { valid_ = false; }

 private:
  int id_;
  PlatformRendererImpl* parent_;
  std::vector<std::unique_ptr<PlatformRenderer>> children_;
  bool valid_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_IMPL_H_

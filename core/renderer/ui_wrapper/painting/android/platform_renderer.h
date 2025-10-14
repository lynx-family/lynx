// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_H_

#include <memory>
#include <vector>

namespace lynx::tasm {

class DisplayList;

// Abstract base class for platform-specific UI rendering operations.
// Provides a common interface for cross-platform UI element management.
class PlatformRenderer {
 public:
  virtual ~PlatformRenderer() = default;

  // Update the display list for this renderer
  virtual void UpdateDisplayList(const DisplayList& display_list) = 0;

  // Add a child renderer
  virtual void AddChild(std::unique_ptr<PlatformRenderer> child) = 0;

  // Remove this renderer from its parent
  virtual void RemoveFromParent() = 0;

  // Get the unique identifier for this renderer
  virtual int GetId() const = 0;

  // Check if this renderer is valid and ready for operations
  virtual bool IsValid() const = 0;
};

// Factory interface for creating platform-specific renderers
class PlatformRendererFactory {
 public:
  virtual ~PlatformRendererFactory() = default;

  // Create a new platform renderer with the given ID
  virtual std::unique_ptr<PlatformRenderer> CreateRenderer(int id) = 0;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_H_

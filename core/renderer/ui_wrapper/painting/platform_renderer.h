// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_H_

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/public/platform_renderer_type.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx::tasm {

class DisplayList;
class PropBundle;
class PlatformExtraBundle;

// Abstract base class for platform-specific UI rendering operations.
// Provides a common interface for cross-platform UI element management.
class PlatformRenderer : public fml::RefCountedThreadSafeStorage {
 public:
  ~PlatformRenderer() override = default;
  // Update the display list for this renderer
  virtual void UpdateDisplayList(DisplayList display_list) = 0;

  // Update attribute bundle for this renderer. Default implementation is
  // provided in PlatformRendererImpl.
  virtual void UpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                bool tends_to_flatten) = 0;
  // Add a child renderer
  virtual void AddChild(fml::RefPtr<PlatformRenderer> child) = 0;

  // Remove this renderer from its parent
  virtual void RemoveFromParent() = 0;

  // Get the unique identifier for this renderer
  virtual int GetId() const = 0;

  virtual const base::InlineVector<fml::RefPtr<PlatformRenderer>,
                                   kChildrenInlineVectorSize>&
  Children() const = 0;

  void ReleaseSelf() const override = 0;
};

// Factory interface for creating platform-specific renderers
class PlatformRendererFactory {
 public:
  virtual ~PlatformRendererFactory() = default;

  // Create a new platform renderer with the given ID
  virtual fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data) = 0;

  virtual fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data) = 0;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_H_

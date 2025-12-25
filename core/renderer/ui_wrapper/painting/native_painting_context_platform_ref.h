// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_

#include <memory>

#include "base/include/value/base_string.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer.h"

namespace lynx {
namespace tasm {

class PlatformRenderer;
class DisplayList;

class NativePaintingCtxPlatformRef : public PaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxPlatformRef(
      std::unique_ptr<PlatformRendererFactory> view_factory);
  ~NativePaintingCtxPlatformRef() override = default;

  void CreatePlatformRenderer(int id, PlatformRendererType type);
  void CreatePlatformExtendedRenderer(int id, const base::String &tag_name);
  void UpdateDisplayList(int id, DisplayList &&display_list);

  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;

 protected:
  void RebuildSubLayers(const fml::RefPtr<PlatformRenderer> &renderer,
                        const base::InlineVector<int, 16> &new_children);

  std::unique_ptr<PlatformRendererFactory> view_factory_;
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformRenderer>, 64>
      renderers_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_

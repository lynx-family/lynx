// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_EPOXY_SHM_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_EPOXY_SHM_IMAGE_REPRESENTATION_H_

#include <memory>
#include <optional>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class EpoxyShmImageRepresentation : public GLImageRepresentation {
 public:
  explicit EpoxyShmImageRepresentation(fml::RefPtr<SharedImageBacking> backing);
  ~EpoxyShmImageRepresentation() override;

  ImageRepresentationType GetType() const override;
  void ConsumeFence(std::unique_ptr<FenceSync>) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool EndWrite() override;

 private:
  std::optional<TextureInfo> GetTexImage() override;
  bool ReleaseTexImage() override;
  std::optional<FramebufferInfo> BindFrameBuffer() override;
  bool UnbindFrameBuffer() override;

  uint32_t texture_id_ = 0;
  uint32_t fbo_id_ = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_EPOXY_SHM_IMAGE_REPRESENTATION_H_

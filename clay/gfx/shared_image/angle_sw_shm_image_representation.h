
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANGLE_SW_SHM_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_ANGLE_SW_SHM_IMAGE_REPRESENTATION_H_

#include <memory>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class FenceSync;

class AngleSwShmImageRepresentation : public SharedImageRepresentation {
 public:
  explicit AngleSwShmImageRepresentation(
      fml::RefPtr<SharedImageBacking> backing);
  ~AngleSwShmImageRepresentation() override;

  ImageRepresentationType GetType() const override;
  void ConsumeFence(std::unique_ptr<FenceSync>) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool EndRead() override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndWrite() override;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANGLE_SW_SHM_IMAGE_REPRESENTATION_H_

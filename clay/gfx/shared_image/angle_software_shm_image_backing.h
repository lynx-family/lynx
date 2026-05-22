// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANGLE_SOFTWARE_SHM_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_ANGLE_SOFTWARE_SHM_IMAGE_BACKING_H_

#include <string>

#include "base/include/fml/task_runner.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

class SharedImageRepresentation;

class AngleSoftwareShmImageBacking : public SharedImageBacking {
 public:
  AngleSoftwareShmImageBacking(PixelFormat pixel_format, skity::Vec2 size);
  ~AngleSoftwareShmImageBacking() override;
  BackingType GetType() const override;

  GraphicsMemoryHandle GetGFXHandle() const override { return shm_buffer_; }

  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;

#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      skity::GPUContext* skity_context) override;
#endif  // ENABLE_SKITY

  void CopyPixelsToShm();

  int GetShmFd() const { return shm_fd_; }

 private:
  GraphicsMemoryHandle shm_buffer_ = nullptr;
  std::string shm_name_;
  int shm_fd_ = -1;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANGLE_SOFTWARE_SHM_IMAGE_BACKING_H_

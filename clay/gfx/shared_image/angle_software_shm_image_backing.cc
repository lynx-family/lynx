// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/angle_software_shm_image_backing.h"

#include <fcntl.h>
#include <sys/mman.h>

#include <tuple>

#include "angle_gl.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/angle_sw_shm_image_representation.h"
#ifndef ENABLE_SKITY
#include "clay/gfx/shared_image/skia_shm_image_representation.h"
#endif

namespace clay {

namespace {

std::tuple<void*, int> CreateShmBuffer(
    SharedImageBacking::PixelFormat pixel_format, skity::Vec2 size) {
  static size_t count = 0;
  std::string shm_name =
      "/angle_shared_image_backing" + std::to_string(getpid()) + "_" +
      std::to_string(pthread_self()) + "_" + std::to_string(count++);
  size_t shm_size = size.x * size.y * 4;

  void* buffer = nullptr;
  int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
  FML_DCHECK(shm_fd != -1);
  ftruncate(shm_fd, shm_size);
  buffer = mmap(NULL, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
  FML_DCHECK(buffer);
  shm_unlink(shm_name.c_str());

  return {buffer, shm_fd};
}

}  // namespace

AngleSoftwareShmImageBacking::AngleSoftwareShmImageBacking(
    PixelFormat pixel_format, skity::Vec2 size)
    : SharedImageBacking(pixel_format, size) {
  std::tuple<void*, int> result = CreateShmBuffer(pixel_format, size);
  shm_buffer_ = std::get<0>(result);
  shm_fd_ = std::get<1>(result);
}

SharedImageBacking::BackingType AngleSoftwareShmImageBacking::GetType() const {
  return BackingType::kAngleShmImage;
}

fml::RefPtr<SharedImageRepresentation>
AngleSoftwareShmImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  ClaySharedImageRepresentationType type = config->type;
  switch (type) {
    case ClaySharedImageRepresentationType::
        kClaySharedImageRepresentationTypeShm: {
      return fml::MakeRefCounted<AngleSwShmImageRepresentation>(fml::Ref(this));
    }
    default: {
      // NOT SUPPORTED.
      FML_LOG(ERROR) << "Unable to call CreateRepresentation with type: "
                     << static_cast<uint32_t>(type);
      return nullptr;
    }
  }
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation>
AngleSoftwareShmImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  return fml::MakeRefCounted<SkiaShmImageRepresentation>(
      fml::MakeRefCounted<AngleSwShmImageRepresentation>(fml::Ref(this)));
}
#else
fml::RefPtr<SkityImageRepresentation>
AngleSoftwareShmImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) override {
  return nullptr;
}
#endif  // ENABLE_SKITY

void AngleSoftwareShmImageBacking::CopyPixelsToShm() {
  glFinish();
  glReadPixels(0, 0, size_.x, size_.y, GL_RGBA, GL_UNSIGNED_BYTE, shm_buffer_);
}

AngleSoftwareShmImageBacking::~AngleSoftwareShmImageBacking() {
  if (munmap(shm_buffer_, size_.x * size_.y * 4) == -1) {
    FML_LOG(ERROR) << "munmap failed";
  }

  close(shm_fd_);
  shm_buffer_ = nullptr;
  shm_fd_ = 0;
}

}  // namespace clay

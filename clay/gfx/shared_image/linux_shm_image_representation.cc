// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/linux_shm_image_representation.h"

#include <cstring>
#include <utility>

#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/epoxy_shm_image_backing.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

LinuxShmImageRepresentation::LinuxShmImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SharedImageRepresentation(std::move(backing)) {}

LinuxShmImageRepresentation::~LinuxShmImageRepresentation() {}

ImageRepresentationType LinuxShmImageRepresentation::GetType() const {
  return ImageRepresentationType::kLinuxShm;
}

void LinuxShmImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  fence_sync->ClientWait();
}

std::unique_ptr<FenceSync> LinuxShmImageRepresentation::ProduceFence() {
  return nullptr;
}

bool LinuxShmImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  memset(out, 0, sizeof(ClaySharedImageReadResult));
  out->struct_size = sizeof(ClaySharedImageReadResult);
  out->type = kClaySharedImageRepresentationTypeShm;
  out->shm_image.struct_size = sizeof(ClaySharedMemoryImage);
  out->shm_image.shm_fd =
      static_cast<EpoxyShmImageBacking*>(GetBacking())->GetShmFd();
  out->shm_image.width = GetBacking()->GetSize().x;
  out->shm_image.height = GetBacking()->GetSize().y;
  out->shm_image.user_data = this;
  this->AddRef();
  out->shm_image.destruction_callback = [](void* user_data) {
    static_cast<LinuxShmImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool LinuxShmImageRepresentation::EndRead() { return true; }

bool LinuxShmImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  memset(out, 0, sizeof(ClaySharedImageWriteResult));
  out->struct_size = sizeof(ClaySharedImageWriteResult);
  out->type = kClaySharedImageRepresentationTypeShm;
  out->shm_image.struct_size = sizeof(ClaySharedMemoryImage);
  out->shm_image.shm_fd =
      static_cast<EpoxyShmImageBacking*>(GetBacking())->GetShmFd();
  out->shm_image.width = GetBacking()->GetSize().x;
  out->shm_image.height = GetBacking()->GetSize().y;
  out->shm_image.user_data = this;
  this->AddRef();
  out->shm_image.destruction_callback = [](void* user_data) {
    static_cast<LinuxShmImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool LinuxShmImageRepresentation::EndWrite() {
  static_cast<EpoxyShmImageBacking*>(GetBacking())->CopyPixelsToShm();
  return true;
}

}  // namespace clay

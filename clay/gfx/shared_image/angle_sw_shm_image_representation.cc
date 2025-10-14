// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/angle_sw_shm_image_representation.h"

#include <cstring>
#include <utility>

#include "clay/gfx/shared_image/angle_software_shm_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

AngleSwShmImageRepresentation::AngleSwShmImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SharedImageRepresentation(std::move(backing)) {}

AngleSwShmImageRepresentation::~AngleSwShmImageRepresentation() {}

ImageRepresentationType AngleSwShmImageRepresentation::GetType() const {
  return ImageRepresentationType::kLinuxShm;
}

void AngleSwShmImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  fence_sync->ClientWait();
}

std::unique_ptr<FenceSync> AngleSwShmImageRepresentation::ProduceFence() {
  return nullptr;
}

bool AngleSwShmImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  memset(out, 0, sizeof(ClaySharedImageReadResult));
  out->struct_size = sizeof(ClaySharedImageReadResult);
  out->type = kClaySharedImageRepresentationTypeShm;
  out->shm_image.struct_size = sizeof(ClaySharedMemoryImage);
  out->shm_image.shm_fd =
      static_cast<AngleSoftwareShmImageBacking*>(GetBacking())->GetShmFd();
  out->shm_image.width = GetBacking()->GetSize().x;
  out->shm_image.height = GetBacking()->GetSize().y;
  out->shm_image.user_data = this;
  this->AddRef();
  out->shm_image.destruction_callback = [](void* user_data) {
    static_cast<AngleSwShmImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool AngleSwShmImageRepresentation::EndRead() { return true; }

bool AngleSwShmImageRepresentation::BeginWrite(
    ClaySharedImageWriteResult* out) {
  memset(out, 0, sizeof(ClaySharedImageWriteResult));
  out->struct_size = sizeof(ClaySharedImageWriteResult);
  out->type = kClaySharedImageRepresentationTypeShm;
  out->shm_image.struct_size = sizeof(ClaySharedMemoryImage);
  out->shm_image.shm_fd =
      static_cast<AngleSoftwareShmImageBacking*>(GetBacking())->GetShmFd();
  out->shm_image.width = GetBacking()->GetSize().x;
  out->shm_image.height = GetBacking()->GetSize().y;
  out->shm_image.user_data = this;
  this->AddRef();
  out->shm_image.destruction_callback = [](void* user_data) {
    static_cast<AngleSwShmImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool AngleSwShmImageRepresentation::EndWrite() {
  static_cast<AngleSoftwareShmImageBacking*>(GetBacking())->CopyPixelsToShm();
  return true;
}

}  // namespace clay

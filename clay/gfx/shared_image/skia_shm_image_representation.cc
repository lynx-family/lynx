// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skia_shm_image_representation.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fstream>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

SkiaShmImageRepresentation::SkiaShmImageRepresentation(
    fml::RefPtr<SharedImageRepresentation> representation)
    : SkiaImageRepresentation(fml::Ref(representation->GetBacking())),
      representation_(representation) {}

SkiaShmImageRepresentation::~SkiaShmImageRepresentation() = default;

sk_sp<SkImage> SkiaShmImageRepresentation::GetSkImage() {
  ClaySharedImageReadResult result;
  if (!representation_->BeginRead(&result)) {
    FML_LOG(ERROR)
        << "BeginRead failed in SkiaShmImageRepresentation::GetSkImage, "
           "returning nullptr.";
    return nullptr;
  }
  FML_DCHECK(result.struct_size == sizeof(result));
  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeShm &&
             result.shm_image.struct_size == sizeof(ClaySharedMemoryImage));

  GrImageInfo info =
      GrImageInfo::Make(GetSize().x, GetSize().y, kRGBA_8888_SkColorType,
                        kPremul_SkAlphaType, SkColorSpace::MakeSRGB());

  void* shm_buffer = mmap(nullptr, GetSize().x * GetSize().y * 4, PROT_READ,
                          MAP_SHARED, result.shm_image.shm_fd, 0);

  size_t rowBytes = GetSize().x * 4;
  SkPixmap pixmap(info, shm_buffer, rowBytes);
  sk_sp<SkImage> sk_image = SkImages::RasterFromPixmapCopy(pixmap);
  auto surface = SkSurface::MakeRaster(info);
  auto canvas = surface->getCanvas();
  canvas->translate(0, sk_image->height());
  canvas->scale(1, -1);
  canvas->drawImage(sk_image, 0, 0);
  return surface->makeImageSnapshot();
}

bool SkiaShmImageRepresentation::EndRead() {
  return representation_->EndRead();
}

void SkiaShmImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  return representation_->ConsumeFence(std::move(fence_sync));
}
std::unique_ptr<FenceSync> SkiaShmImageRepresentation::ProduceFence() {
  return representation_->ProduceFence();
}

}  // namespace clay

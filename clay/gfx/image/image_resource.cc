// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_resource.h"

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/image/image.h"
#include "clay/gfx/image/image_resource_client.h"

namespace clay {

ImageResource::ImageResource(std::shared_ptr<Image> image, bool owned_by_raster)
    : owned_by_raster_(owned_by_raster), image_(image) {
  FML_DCHECK(image_);
  image_->AccessorCreated(this);
}

ImageResource::ImageResource(const ImageResource& other)
    : image_(other.image_) {
  FML_DCHECK(image_);
  image_->AccessorCreated(this);
}

ImageResource::~ImageResource() { ReleaseImage(); }

bool ImageResource::IsSVG() const {
#if defined(ENABLE_SVG)
  return image_ && image_->IsSVG();
#else
  return false;
#endif
}

void ImageResource::ReleaseImage() {
  if (image_) {
    image_->AccessorDestroyed(this);
    image_ = nullptr;
  }
}

std::string ImageResource::GetUrl() const {
  if (image_) {
    return image_->GetUrl();
  }
  return "";
}

void ImageResource::SetResourceReleased() { image_ = nullptr; }

fml::RefPtr<GraphicsImage> ImageResource::GetGraphicsImage(
    DecodePriority priority) const {
  if (image_for_test_) {
    return image_for_test_;
  }

  if (image_ && image_->IsDeferred() && !image_->DecodeWithPriority()) {
    return GraphicsImage::MakeLazy(image_->shared_from_this());
  }

  if (image_) {
    return image_->GetGraphicsImage(priority);
  }
  return nullptr;
}

void ImageResource::SetTestImage(fml::RefPtr<GraphicsImage> img) {
  image_for_test_ = img;
}

int ImageResource::GetWidth() const {
  if (image_) {
    return image_->GetWidth();
  }
  return 0;
}

int ImageResource::GetHeight() const {
  if (image_) {
    return image_->GetHeight();
  }
  return 0;
}

void ImageResource::AddImageResourceClient(ImageResourceClient* client) {
  if (!client) {
    return;
  }
  clients_.emplace(client);
}

void ImageResource::RemoveImageResourceClient(ImageResourceClient* client) {
  if (!client) {
    return;
  }
  clients_.erase(client);
}

void ImageResource::DecodeFinish(bool success) {
  for (auto client : clients_) {
    if (image_ && image_->UseTextureBackend() &&
        !image_->DecodeWithPriority()) {
      client->RequestRenderImage(this, success);
    }
    client->DecodeImageFinish(success);
  }
}

void ImageResource::UploadFinish(bool success) {
  for (auto client : clients_) {
    if (image_ && image_->UseTextureBackend()) {
      client->RequestRenderImage(this, success);
    }
  }
}

void ImageResource::RegisterUploadTask(OneShotCallback<>&& task, int image_id) {
  for (auto client : clients_) {
    client->RegisterUploadTask(std::move(task), image_id);
  }
}

void ImageResource::AnimationAdvanced() {
  for (auto client : clients_) {
    client->OnImageChanged();
  }
}

bool ImageResource::ShouldPauseAnimation() {
  for (auto* client : clients_) {
    if (client->WillRenderImage()) {
      return false;
    }
  }
  return true;
}

void ImageResource::OnStartPlay() {
  for (auto* client : clients_) {
    client->OnStartPlay();
  }
}

void ImageResource::OnCurrentLoopComplete() {
  for (auto* client : clients_) {
    client->OnCurrentLoopComplete();
  }
}

void ImageResource::OnFinalLoopComplete() {
  for (auto* client : clients_) {
    client->OnFinalLoopComplete();
  }
}

}  // namespace clay

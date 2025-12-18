// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_RESOURCE_H_
#define CLAY_GFX_IMAGE_IMAGE_RESOURCE_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "clay/gfx/image/skimage_holder.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/one_shot_callback.h"
#include "clay/ui/rendering/decode_utils.h"

namespace clay {

class Image;
class ImageResourceClient;

// Image reference diagram:
//
// Outer --> ImageResource [ GetSkImage ]
//              |___Image [ Animation & Manager data ]
//                    |___ImageProducer [ Decoding and caching ]
//                          |__-->SkImageHolder [ GPU Resource cache ]
//
// Component/RenderObject will hold ImageResource to access or release SkImage.
class ImageResource {
 public:
  explicit ImageResource(std::shared_ptr<Image>, bool owned_by_raster = false);
  ImageResource(const ImageResource& other);
  ~ImageResource();

  bool IsSVG() const;
  std::string GetUrl() const;
  Image* GetImage() const { return image_.get(); }
  std::shared_ptr<Image> GetImageShared() const { return image_; }
  fml::RefPtr<GraphicsImage> GetGraphicsImage(
      DecodePriority priority = DecodePriority::kImmediate) const;
  int GetWidth() const;
  int GetHeight() const;
  void SetResourceReleased();

  void AddImageResourceClient(ImageResourceClient*);
  void RemoveImageResourceClient(ImageResourceClient*);

  void AnimationAdvanced();
  void DecodeFinish(bool success);
  void UploadFinish(bool success);
  void RegisterUploadTask(OneShotCallback<>&& task, int image_id);
  bool ShouldPauseAnimation();

  bool OwnedByRaster() const { return owned_by_raster_; }

  void OnStartPlay();
  void OnCurrentLoopComplete();
  void OnFinalLoopComplete();

  // Only for testing
  void SetTestImage(fml::RefPtr<GraphicsImage> img);

  const std::unordered_set<ImageResourceClient*>& GetClients() const {
    return clients_;
  }

  size_t GetGraphicsImageAllocSize() const;

 private:
  void ReleaseImage();
  bool owned_by_raster_;
  fml::RefPtr<GraphicsImage> image_for_test_;
  std::shared_ptr<Image> image_ = nullptr;
  std::unordered_set<ImageResourceClient*> clients_;
};
}  // namespace clay
#endif  // CLAY_GFX_IMAGE_IMAGE_RESOURCE_H_

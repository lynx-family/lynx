// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_RESOURCE_CLIENT_H_
#define CLAY_GFX_IMAGE_IMAGE_RESOURCE_CLIENT_H_

namespace clay {

class ImageResource;

class ImageResourceClient {
 public:
  virtual bool WillRenderImage() = 0;
  virtual void RequestRenderImage(ImageResource* image_resource,
                                  bool success) = 0;
  virtual void OnImageChanged() = 0;
  virtual void DecodeImageFinish(bool success) {}

  virtual void OnStartPlay() {}
  virtual void OnCurrentLoopComplete() {}
  virtual void OnFinalLoopComplete() {}

 protected:
  virtual ~ImageResourceClient() = default;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_RESOURCE_CLIENT_H_

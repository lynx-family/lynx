// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_RESOURCE_CLIENT_H_
#define CLAY_GFX_IMAGE_IMAGE_RESOURCE_CLIENT_H_

#include <string>

#include "clay/shell/common/one_shot_callback.h"
#include "clay/ui/rendering/decode_utils.h"

namespace clay {

class ImageResource;

class ImageResourceClient {
 public:
  virtual bool WillRenderImage() = 0;
  virtual void RequestRenderImage(ImageResource* image_resource,
                                  bool success) = 0;
  virtual void OnImageChanged() = 0;
  virtual void DecodeImageFinish(bool success, const std::string& url) {}

  virtual void RegisterUploadTask(OneShotCallback<>&& task, int image_id) {}

  virtual void OnStartPlay() {}
  virtual void OnCurrentLoopComplete() {}
  virtual void OnFinalLoopComplete() {}

  virtual DecodePriority GetDecodePriority() {
    return DecodePriority::kImmediate;
  }

 protected:
  virtual ~ImageResourceClient() = default;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_RESOURCE_CLIENT_H_

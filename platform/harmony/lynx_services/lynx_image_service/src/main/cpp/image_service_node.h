// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_SERVICE_NODE_H_
#define PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_SERVICE_NODE_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "imageknife.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace service {

class ImageServiceHarmony;

class ImageServiceNode : public tasm::harmony::ImageNode {
 public:
  explicit ImageServiceNode(ImageServiceHarmony* service);
  ~ImageServiceNode() override;
  ArkUI_NodeHandle GetNodeHandle() override;
  void FetchImage(tasm::harmony::ImageRequestInfo info) override;
  void InitImageLoadListener(
      const std::weak_ptr<tasm::harmony::ImageLoadListener>& listener) override;
  void InitAnimationListener(
      const std::weak_ptr<tasm::harmony::AnimationListener>& listener) override;
  void StartAnimation() override;
  void StopAnimation() override;
  void PauseAnimation() override;
  void ResumeAnimation() override;
  void UpdateAutoPlay(bool autoplay) override;
  void UpdateLoopCount(int count) override;
  void Clear() override;

 private:
  void UpdateImageSource(const std::string& url,
                         ImageKnifePro::ImageSource& source);
  std::shared_ptr<ImageKnifePro::ImageKnifeNode> image_knife_node_;
  std::shared_ptr<ImageKnifePro::ImageKnifeOption> image_knife_option_;
  std::shared_ptr<ImageKnifePro::AnimatorOption> image_knife_animator_option_;
  std::weak_ptr<tasm::harmony::ImageLoadListener> load_listener_;
  std::weak_ptr<tasm::harmony::AnimationListener> animation_listener_;
  ImageServiceHarmony* service_;
};

}  // namespace service
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_SERVICES_LYNX_IMAGE_SERVICE_SRC_MAIN_CPP_IMAGE_SERVICE_NODE_H_

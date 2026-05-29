// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_node.h"

#include <utility>

#include "base/include/string/string_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_harmony.h"
#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_transform.h"

namespace lynx {
namespace service {

static bool IsResource(const std::string& url) {
  return base::BeginsWith(url, tasm::harmony::image::kResourceScheme);
}

static bool IsMedia(const std::string& url) {
  return base::BeginsWith(url, tasm::harmony::image::kResourceBaseMedia);
}

static bool IsRawFile(const std::string& url) {
  return base::BeginsWith(url, tasm::harmony::image::kResourceRawFile);
}

static tasm::harmony::LynxImageOrigin GetImageOrigin(
    ImageKnifePro::ImageSourceFrom from) {
  tasm::harmony::LynxImageOrigin origin =
      tasm::harmony::LynxImageOrigin::kUnknown;
  if (from == ImageKnifePro::ImageSourceFrom::FROM_MEMORY_CACHE) {
    origin = tasm::harmony::LynxImageOrigin::kMemoryDecoded;
  } else if (from == ImageKnifePro::ImageSourceFrom::FROM_FILE_CACHE) {
    origin = tasm::harmony::LynxImageOrigin::kDisk;
  } else if (from == ImageKnifePro::ImageSourceFrom::FROM_LOCAL_FILE) {
    origin = tasm::harmony::LynxImageOrigin::kLocal;
  } else if (from == ImageKnifePro::ImageSourceFrom::FROM_NETWORK_DOWNLOAD) {
    origin = tasm::harmony::LynxImageOrigin::kNetwork;
  }
  return origin;
}

ImageServiceNode::ImageServiceNode(ImageServiceHarmony* service)
    : ImageNode(), service_(service) {
  image_knife_option_ = std::make_shared<ImageKnifePro::ImageKnifeOption>();
  image_knife_animator_option_ =
      std::make_shared<ImageKnifePro::AnimatorOption>();
  image_knife_node_ =
      ImageKnifePro::ImageKnifeNode::CreateImageKnifeNode(image_knife_option_);
}

ImageServiceNode::~ImageServiceNode() { image_knife_node_->DisposeNode(); }

ArkUI_NodeHandle ImageServiceNode::GetNodeHandle() {
  return image_knife_node_->GetHandle();
}

void ImageServiceNode::UpdateImageSource(
    ImageServiceHarmony* service,
    const std::shared_ptr<ImageKnifePro::ImageKnifeOption>& option,
    const std::string& url, ImageKnifePro::ImageSource& source) {
  if (IsResource(url)) {
    if (!option->context.resourceManager && service) {
      option->context.resourceManager = service->CreateNativeResourceManager();
    }
    std::string param;
    if (IsRawFile(url)) {
      param = url.substr(std::strlen(tasm::harmony::image::kResourceRawFile));
    } else if (IsMedia(url)) {
      param = url.substr(std::strlen(tasm::harmony::image::kResourceBaseMedia));
      size_t file_format = param.find_last_of('.');
      if (file_format != std::string::npos) {
        param = param.substr(0, file_format);
      }
    }
    ImageKnifePro::Resource resource{
        .id = -1,
        .param = param,
    };
    source.SetResource(resource);
  } else {
    source.SetString(url);
  }
}

void ImageServiceNode::FetchImage(tasm::harmony::ImageRequestInfo info) {
  image_knife_option_->objectFit = info.mode;
  image_knife_option_->downSampling =
      info.downsampling ? ImageKnifePro::DownSamplingStrategy::FIT_CENTER_MEMORY
                        : ImageKnifePro::DownSamplingStrategy::DEFAULT;
  UpdateImageSource(service_, image_knife_option_, info.url,
                    image_knife_option_->loadSrc);
  UpdateImageSource(service_, image_knife_option_, info.placeholder,
                    image_knife_option_->placeholderSrc);
  image_knife_option_->fallbackUrls = std::move(info.fallback_urls);
  if (info.processors.empty()) {
    image_knife_option_->transformation = nullptr;
  } else {
    image_knife_option_->transformation =
        std::make_shared<ImageTransform>(std::move(info.processors));
  }
  image_knife_node_->Update(image_knife_option_);
}

void ImageServiceNode::InitImageLoadListener(
    const std::weak_ptr<tasm::harmony::ImageLoadListener>& listener) {
  if (!load_listener_.expired()) {
    return;
  }
  load_listener_ = listener;
  image_knife_option_->onLoadListener =
      std::make_shared<ImageKnifePro::OnLoadCallBack>();
  image_knife_option_->onLoadListener->onLoadSuccess =
      [weak_listener = load_listener_](const ImageKnifePro::ImageInfo& info) {
        auto listener = weak_listener.lock();
        if (!listener) {
          return;
        }
        listener->OnImageLoadSuccess(info.imageWidth, info.imageHeight);
        if (listener->NeedMonitorInfo()) {
          tasm::harmony::ImageMonitorInfo monitorInfo{
              .load_start = info.timeInfo.requestStartTime,
              .load_finish = info.timeInfo.requestEndTime,
              .origin = GetImageOrigin(info.imageSourceFrom)};
          listener->OnImageMonitorInfo(monitorInfo);
        }
      };
  image_knife_option_->onLoadListener->onLoadFailed =
      [weak_listener = load_listener_](const std::string& err,
                                       const ImageKnifePro::ImageInfo& info) {
        auto listener = weak_listener.lock();
        if (!listener) {
          return;
        }
        auto error_code = static_cast<int>(info.errorInfo.code);
        listener->OnImageLoadFailure(error_code, err);
      };
}

void ImageServiceNode::InitAnimationListener(
    const std::weak_ptr<tasm::harmony::AnimationListener>& listener) {
  if (!animation_listener_.expired()) {
    return;
  }
  animation_listener_ = listener;
  image_knife_animator_option_->onStart = [weak_listener =
                                               animation_listener_]() {
    auto listener = weak_listener.lock();
    if (!listener) {
      return;
    }
    listener->OnAnimationStart();
  };
  image_knife_animator_option_->onFinish = [weak_listener =
                                                animation_listener_]() {
    auto listener = weak_listener.lock();
    if (!listener) {
      return;
    }
    listener->OnAnimationFinish();
  };
  image_knife_animator_option_->onRepeat = [weak_listener =
                                                animation_listener_]() {
    auto listener = weak_listener.lock();
    if (!listener) {
      return;
    }
    listener->OnAnimationRepeat();
  };
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
}

void ImageServiceNode::StartAnimation() {
  if (image_knife_animator_option_->state != ARKUI_ANIMATION_STATUS_RUNNING) {
    image_knife_animator_option_->state = ARKUI_ANIMATION_STATUS_RUNNING;
    image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
  }
}

void ImageServiceNode::StopAnimation() {
  image_knife_animator_option_->state = ARKUI_ANIMATION_STATUS_STOPPED;
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
}

void ImageServiceNode::PauseAnimation() {
  image_knife_animator_option_->state = ARKUI_ANIMATION_STATUS_PAUSED;
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
}

void ImageServiceNode::ResumeAnimation() {
  if (image_knife_animator_option_->state != ARKUI_ANIMATION_STATUS_RUNNING) {
    image_knife_animator_option_->state = ARKUI_ANIMATION_STATUS_RUNNING;
    image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
  }
}

void ImageServiceNode::UpdateAutoPlay(bool autoplay) {
  image_knife_animator_option_->state = autoplay
                                            ? ARKUI_ANIMATION_STATUS_RUNNING
                                            : ARKUI_ANIMATION_STATUS_STOPPED;
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
}

void ImageServiceNode::UpdateLoopCount(int count) {
  // Lynx uses 0 for infinite loop, while the underlying image library uses -1.
  image_knife_animator_option_->iterations = (count == 0) ? -1 : count;
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
}

void ImageServiceNode::Clear() {
  image_knife_node_->Clear();
  ImageKnifePro::ImageKnife::GetInstance().CancelRequest(
      image_knife_node_->GetImageKnifeRequest());
}

}  // namespace service
}  // namespace lynx

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_service_node.h"

#include <utility>

#include "base/include/string/string_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_services/lynx_image_service/src/main/cpp/image_knife_option_compat.h"
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
  auto option = std::make_shared<ImageKnifePro::ImageKnifeOption>();
  ImageKnifeOptionCompat::SetEnableVisibleAreaControl(option.get(), false);
  image_knife_animator_option_ =
      std::make_shared<ImageKnifePro::AnimatorOption>();
  image_knife_node_ =
      ImageKnifePro::ImageKnifeNode::CreateImageKnifeNode(std::move(option));
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
  // ImageKnife retains options for asynchronous requests. Use a new instance
  // so updating this request cannot mutate an in-flight request.
  auto option = std::make_shared<ImageKnifePro::ImageKnifeOption>();
  option->onLoadListener = image_knife_load_listener_;

  option->objectFit = info.mode;
  option->downSampling =
      info.downsampling ? ImageKnifePro::DownSamplingStrategy::FIT_CENTER_MEMORY
                        : ImageKnifePro::DownSamplingStrategy::DEFAULT;
  UpdateImageSource(service_, option, info.url, option->loadSrc);
  UpdateImageSource(service_, option, info.placeholder, option->placeholderSrc);
  option->fallbackUrls = std::move(info.fallback_urls);
  option->fileCacheName = std::move(info.file_cache_name);
  if (!info.processors.empty()) {
    option->transformation =
        std::make_shared<ImageTransform>(std::move(info.processors));
  }
  ImageKnifeOptionCompat::Apply(option.get(), info);

  image_knife_node_->Update(std::move(option));
}

void ImageServiceNode::InitImageLoadListener(
    const std::weak_ptr<tasm::harmony::ImageLoadListener>& listener) {
  image_knife_load_listener_ =
      std::make_shared<ImageKnifePro::OnLoadCallBack>();
  image_knife_load_listener_->onLoadSuccess =
      [weak_listener = listener](const ImageKnifePro::ImageInfo& info) {
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
  image_knife_load_listener_->onLoadFailed =
      [weak_listener = listener](const std::string& err,
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
  image_knife_animator_option_->onStart = [weak_listener = listener]() {
    auto listener = weak_listener.lock();
    if (!listener) {
      return;
    }
    listener->OnAnimationStart();
  };
  image_knife_animator_option_->onFinish = [weak_listener = listener]() {
    auto listener = weak_listener.lock();
    if (!listener) {
      return;
    }
    listener->OnAnimationFinish();
  };
  image_knife_animator_option_->onRepeat = [weak_listener = listener]() {
    auto listener = weak_listener.lock();
    if (!listener) {
      return;
    }
    listener->OnAnimationRepeat();
  };
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
}

void ImageServiceNode::StartAnimation() {
  image_knife_animator_option_->state = ARKUI_ANIMATION_STATUS_RUNNING;
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
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
  image_knife_animator_option_->state = ARKUI_ANIMATION_STATUS_RUNNING;
  image_knife_node_->UpdateAnimatorOption(image_knife_animator_option_);
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

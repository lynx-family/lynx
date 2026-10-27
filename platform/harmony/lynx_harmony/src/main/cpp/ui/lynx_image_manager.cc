// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/table.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/image_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_effect_processor.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/image_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

class CapInsetsImageData final : public ImageData {
 public:
  explicit CapInsetsImageData(OH_PixelmapNative* pixel_map)
      : pixel_map_(pixel_map) {}

  ~CapInsetsImageData() override { OH_PixelmapNative_Release(pixel_map_); }

  OH_PixelmapNative* Pixelmap() const override { return pixel_map_; }
  uint32_t FrameCount() const override { return 1; }
  OH_PixelmapNative** PixelmapList() const override { return nullptr; }
  int* DelayTimeList() const override { return nullptr; }

 private:
  OH_PixelmapNative* pixel_map_;
};

std::shared_ptr<ImageProcessor> CreateCapInsetsProcessor(
    const ImagePaintInfo& paint_info, float width, float height,
    float density) {
  std::vector<std::string> values;
  base::SplitString(paint_info.cap_insets.str(), ' ', values);
  if (values.size() != 4) {
    return nullptr;
  }
  float insets[4];
  for (size_t i = 0; i < values.size(); ++i) {
    const auto& value = values[i];
    if (!base::EndsWith(value, "px") ||
        !base::StringToFloat(value.substr(0, value.size() - 2), insets[i])) {
      return nullptr;
    }
  }
  // CUI supplies content-box dimensions, so padding is already excluded.
  LynxImageEffectProcessor::CapInsetParams params{
      insets[3],
      insets[0],
      insets[1],
      insets[2],
      paint_info.cap_insets_scale,
      {width, height, 0.f, 0.f, 0.f, 0.f, density}};
  return std::make_shared<LynxImageEffectProcessor>(
      LynxImageEffectProcessor::ImageEffect::kCapInsets, params);
}

ImageDrawable::ImageMode ToImageDrawableMode(ImageFitMode mode) {
  switch (mode) {
    case ImageFitMode::kAspectFit:
      return ImageDrawable::ImageMode::kAspectFit;
    case ImageFitMode::kAspectFill:
      return ImageDrawable::ImageMode::kAspectFill;
    case ImageFitMode::kCenter:
      return ImageDrawable::ImageMode::kCenter;
    case ImageFitMode::kScaleToFill:
      return ImageDrawable::ImageMode::kScaleToFill;
  }
  return ImageDrawable::ImageMode::kScaleToFill;
}

}  // namespace

LynxImageManager::LynxImageManager(std::weak_ptr<LynxContext> context)
    : context_(std::move(context)) {}

LynxImageManager::~LynxImageManager() {
  if (drawable_) {
    drawable_->RemoveImageAnimationListener(this);
  }
}

void LynxImageManager::UpdatePaintInfo(const ImagePaintInfo& paint_info,
                                       uint32_t changes) {
  constexpr uint32_t paint_changes = ImagePaintInfo::kModeChanged |
                                     ImagePaintInfo::kBlurRadiusChanged |
                                     ImagePaintInfo::kTintColorChanged;
  constexpr uint32_t playback_changes =
      ImagePaintInfo::kAutoplayChanged | ImagePaintInfo::kLoopCountChanged;
  if (changes & paint_changes) {
    dirty_flags_ |= kPaintDirty;
  }
  if (changes & ImagePaintInfo::kAutoplayChanged) {
    dirty_flags_ |= kAutoplayDirty;
  }
  if (changes & ImagePaintInfo::kLoopCountChanged) {
    dirty_flags_ |= kLoopCountDirty;
  }
  if (changes &
      (ImagePaintInfo::kAllChanged & ~(paint_changes | playback_changes))) {
    dirty_flags_ |= kRequestDirty;
  }
  if (changes & ImagePaintInfo::kModeChanged) {
    has_bounds_ = false;
  }
  paint_info_ = paint_info;
}

void LynxImageManager::RequestImage(int32_t sign, std::string src, float width,
                                    float height, int32_t event_mask) {
  sign_ = sign;
  event_mask_ = event_mask;

  if (source_ != src || ((!paint_info_.cap_insets.empty() ||
                          paint_info_.auto_size || paint_info_.downsampling) &&
                         (view_width_ != width || view_height_ != height))) {
    dirty_flags_ |= kRequestDirty;
  }
  source_ = src;
  view_width_ = width;
  view_height_ = height;
  const uint8_t dirty = dirty_flags_;
  dirty_flags_ = 0;
  if (drawable_) {
    if (dirty & kPaintDirty) {
      ApplyPaintInfo();
    }
    if (dirty & kLoopCountDirty) {
      drawable_->UpdateLoopCount(std::max(paint_info_.loop_count, 0));
    }
    if ((dirty & kAutoplayDirty) && !(dirty & kRequestDirty)) {
      if (paint_info_.autoplay) {
        drawable_->StartAnimation();
      } else {
        drawable_->StopAnimation();
      }
    }
    if (dirty & (kPaintDirty | kAutoplayDirty | kLoopCountDirty)) {
      drawable_->NotifyInvalidationObserver();
    }
  }
  if (!(dirty & kRequestDirty)) {
    return;
  }
  const uint64_t generation = ++request_generation_;
  image_.reset();
  if (drawable_) {
    drawable_->StopAnimation();
    drawable_->ResetContent();
    drawable_->NotifyInvalidationObserver();
  }
  if (src.empty()) {
    return;
  }
  auto* image_service = UIOwner::image_service;
  if (image_service == nullptr) {
    dirty_flags_ |= kRequestDirty;
    LOGE("LynxImageManager image service is null");
    return;
  }

  ImageRequestInfo request{.url = std::move(src)};

  std::shared_ptr<ImageProcessor> cap_insets_processor;
  if (!paint_info_.cap_insets.empty() && width > 0.f && height > 0.f) {
    if (auto context = context_.lock()) {
      cap_insets_processor = CreateCapInsetsProcessor(
          paint_info_, width, height, context->ScaledDensity());
    }
  }
  if ((event_mask_ & image::kFlagImageLoadStartEvent) != 0) {
    if (auto context = context_.lock()) {
      auto detail = lepus::Dictionary::Create();
      detail->SetValue("url", request.url);
      context->SendEvent(CustomEvent{sign_, image::kLoadStartEventName,
                                     "detail", lepus_value(detail)});
    }
  }
  image_service->DecodeImage(
      request,
      [weak_self = weak_from_this(), cap_insets_processor,
       generation](const std::shared_ptr<ImageData>& image) {
        auto self = weak_self.lock();
        if (self == nullptr) {
          return;
        }
        self->RunForRequest(generation, [image, cap_insets_processor](
                                            LynxImageManager& manager) {
          manager.image_ = image;

          // Cap-insets processing depends on the size of this request.
          if (image && image->FrameCount() == 1 && image->Pixelmap() &&
              cap_insets_processor) {
            if (auto* processed =
                    cap_insets_processor->Process(image->Pixelmap())) {
              manager.image_ = std::make_shared<CapInsetsImageData>(processed);
            }
          }
          manager.ApplyImage();
          if (manager.drawable_) {
            manager.drawable_->NotifyInvalidationObserver();
          }
        });
      },
      [weak_self = weak_from_this(), generation](float image_width,
                                                 float image_height) {
        if (auto self = weak_self.lock()) {
          self->RunForRequest(generation, [=](LynxImageManager& manager) {
            manager.OnImageLoadSuccess(image_width, image_height,
                                       manager.view_width_,
                                       manager.view_height_);
          });
        }
      },
      [weak_self = weak_from_this(), generation](
          int error_code, const std::string& error_message) {
        if (auto self = weak_self.lock()) {
          self->RunForRequest(generation, [error_code, error_message](
                                              LynxImageManager& manager) {
            manager.dirty_flags_ |= kRequestDirty;
            manager.OnImageLoadFailure(error_code, error_message);
          });
        }
      });
}

void LynxImageManager::RunForRequest(
    uint64_t generation, std::function<void(LynxImageManager&)> callback) {
  if (auto context = context_.lock()) {
    context->RunOnUIThread([weak_self = weak_from_this(), generation,
                            callback = std::move(callback)]() {
      if (auto self = weak_self.lock();
          self && self->request_generation_ == generation) {
        callback(*self);
      }
    });
  }
}

void LynxImageManager::SetTarget(const std::weak_ptr<UIBase>& target) {
  if (drawable_ != nullptr || target.expired()) {
    return;
  }
  drawable_ = std::make_unique<ImageDrawable>(target);
  if ((event_mask_ & image::kFlagImageAnimationEvent) != 0) {
    drawable_->AddImageAnimationListener(this);
  }
  ApplyPaintInfo();
  ApplyImage();
}

void LynxImageManager::UpdateBounds(float width, float height,
                                    float scale_density) {
  if (!paint_info_.is_background &&
      paint_info_.mode == ImageFitMode::kScaleToFill) {
    // Match RSImage's FILL destination size in physical pixels.
    // Clip and translation remain based on the original content box.
    width = std::ceil(width * scale_density);
    height = std::ceil(height * scale_density);
    scale_density = 1.f;
  }
  const float physical_width = width * scale_density;
  const float physical_height = height * scale_density;
  if (drawable_ == nullptr ||
      (has_bounds_ && base::FloatsEqual(width_, physical_width) &&
       base::FloatsEqual(height_, physical_height) &&
       base::FloatsEqual(scale_density_, scale_density))) {
    return;
  }
  scale_density_ = scale_density;
  width_ = physical_width;
  height_ = physical_height;
  has_bounds_ = true;
  drawable_->UpdateBounds(0.f, 0.f, width, height, 0.f, 0.f, 0.f, 0.f,
                          scale_density);
}

void LynxImageManager::Draw(OH_Drawing_Canvas* canvas) {
  if (drawable_ != nullptr && canvas != nullptr) {
    drawable_->Render(canvas);
  }
}

uint32_t LynxImageManager::GetImageWidth() const {
  return drawable_ && drawable_->HasContent() ? drawable_->GetImageWidth() : 0;
}

uint32_t LynxImageManager::GetImageHeight() const {
  return drawable_ && drawable_->HasContent() ? drawable_->GetImageHeight() : 0;
}

void LynxImageManager::onAnimationStart() {
  auto context = context_.lock();
  if (context && (event_mask_ & image::kFlagImageStartPlayEvent) != 0) {
    context->SendEvent(CustomEvent{sign_, image::kStartPlayEventName, "detail",
                                   lepus_value()});
  }
}

void LynxImageManager::onAnimationRepeat() {
  auto context = context_.lock();
  if (context && (event_mask_ & image::kFlagImageCurrentLoopEvent) != 0) {
    context->SendEvent(CustomEvent{sign_, image::kCurrentLoopEventName,
                                   "detail", lepus_value()});
  }
}

void LynxImageManager::onAnimationStop() {
  auto context = context_.lock();
  if (!context) {
    return;
  }
  if ((event_mask_ & image::kFlagImageCurrentLoopEvent) != 0) {
    context->SendEvent(CustomEvent{sign_, image::kCurrentLoopEventName,
                                   "detail", lepus_value()});
  }
  if ((event_mask_ & image::kFlagImageFinalLoopEvent) != 0) {
    context->SendEvent(CustomEvent{sign_, image::kFinalLoopEventName, "detail",
                                   lepus_value()});
  }
}

void LynxImageManager::ApplyPaintInfo() {
  if (drawable_ == nullptr) {
    return;
  }
  drawable_->UpdateMode(ToImageDrawableMode(paint_info_.mode));
  drawable_->UpdateLoopCount(std::max(paint_info_.loop_count, 0));
  drawable_->UpdateBlurRadius(0.f);
  drawable_->ClearTintColor();
  if (!paint_info_.blur_radius.empty()) {
    if (auto context = context_.lock()) {
      const auto& blur_radius = paint_info_.blur_radius.str();
      CSSStringParser parser(blur_radius.data(),
                             static_cast<uint32_t>(blur_radius.size()), {});
      CSSValue radius;
      parser.ParseLengthTo(radius);
      if (!radius.IsEmpty()) {
        drawable_->UpdateBlurRadius(radius.AsNumber() *
                                    context->ScaledDensity());
      }
    }
  }
  if (!paint_info_.tint_color.empty()) {
    CSSColor color;
    if (CSSColor::Parse(paint_info_.tint_color.str(), color)) {
      drawable_->UpdateTintColor(color.Cast());
    }
  }
}

void LynxImageManager::ApplyImage() {
  if (drawable_ == nullptr || image_ == nullptr) {
    return;
  }
  drawable_->UpdateDrawCurrent(image_, /*prepare_draw_resources=*/true);
  if (paint_info_.autoplay) {
    drawable_->StartAnimation();
  }
}

void LynxImageManager::OnImageLoadSuccess(float image_width, float image_height,
                                          float view_width, float view_height) {
  AutoSizeIfNeeded(image_width, image_height, view_width, view_height);
  auto context = context_.lock();
  if (context == nullptr || (event_mask_ & image::kFlagImageLoadEvent) == 0) {
    return;
  }
  auto detail = lepus::Dictionary::Create();
  detail->SetValue(image::kLoadEventImageWidth, image_width);
  detail->SetValue(image::kLoadEventImageHeight, image_height);
  context->SendEvent(
      CustomEvent{sign_, image::kLoadEventName, "detail", lepus_value(detail)});
}

void LynxImageManager::AutoSizeIfNeeded(float image_width, float image_height,
                                        float view_width, float view_height) {
  auto context = context_.lock();
  if (context == nullptr || !paint_info_.auto_size || image_width <= 0.f ||
      image_height <= 0.f) {
    return;
  }
  context->PostTaskOnUIThread([weak_self = weak_from_this(), image_width,
                               image_height, view_width, view_height]() {
    auto self = weak_self.lock();
    if (!self) {
      return;
    }
    auto context = self->context_.lock();
    if (context == nullptr) {
      return;
    }
    context->FindShadowNodeAndRunTask(
        self->sign_, [image_width, image_height, view_width,
                      view_height](ShadowNode* shadow_node) {
          if (shadow_node == nullptr) {
            return;
          }
          const std::string tag = shadow_node->Tag();
          if (tag != "image" && tag != "filter-image") {
            return;
          }
          static_cast<ImageShadowNode*>(shadow_node)
              ->JustSize(true, image_width, image_height, view_width,
                         view_height);
        });
  });
}

void LynxImageManager::OnImageLoadFailure(int32_t error_code,
                                          const std::string& error_message) {
  auto context = context_.lock();
  if (context == nullptr || (event_mask_ & image::kFlagImageErrorEvent) == 0) {
    return;
  }
  auto detail = lepus::Dictionary::Create();
  detail->SetValue(image::kErrorEventCode, error_code);
  detail->SetValue(image::kErrorEventMsg, error_message);
  context->SendEvent(CustomEvent{sign_, image::kErrorEventName, "detail",
                                 lepus_value(detail)});
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

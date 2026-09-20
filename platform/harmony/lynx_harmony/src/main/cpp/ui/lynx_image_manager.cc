// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"

#include <algorithm>
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
    case ImageFitMode::kScaleToFill:
      return ImageDrawable::ImageMode::kScaleToFill;
  }
  return ImageDrawable::ImageMode::kScaleToFill;
}

}  // namespace

LynxImageManager::LynxImageManager(std::weak_ptr<LynxContext> context)
    : context_(std::move(context)) {}

LynxImageManager::~LynxImageManager() = default;

void LynxImageManager::UpdatePaintInfo(const ImagePaintInfo& paint_info) {
  paint_info_ = paint_info;
}

void LynxImageManager::RequestImage(int32_t sign, std::string src, float width,
                                    float height, int32_t event_mask) {
  sign_ = sign;
  event_mask_ = event_mask;
  if (src.empty()) {
    return;
  }
  auto* image_service = UIOwner::image_service;
  if (image_service == nullptr) {
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
  image_service->DecodeImage(
      request,
      [weak_self = weak_from_this(),
       cap_insets_processor](const std::shared_ptr<ImageData>& image) {
        auto self = weak_self.lock();
        if (self == nullptr) {
          return;
        }
        self->image_ = image;
        // Match LynxImageHelper: apply image effects only to static images.
        if (image && image->FrameCount() == 1 && image->Pixelmap() &&
            cap_insets_processor) {
          if (auto* processed =
                  cap_insets_processor->Process(image->Pixelmap())) {
            self->image_ = std::make_shared<CapInsetsImageData>(processed);
          }
        }
        self->ApplyImage();
      },
      [weak_self = weak_from_this(), width, height](float image_width,
                                                    float image_height) {
        if (auto self = weak_self.lock()) {
          self->OnImageLoadSuccess(image_width, image_height, width, height);
        }
      },
      [weak_self = weak_from_this()](int error_code,
                                     const std::string& error_message) {
        if (auto self = weak_self.lock()) {
          self->OnImageLoadFailure(error_code, error_message);
        }
      });
}

void LynxImageManager::SetTarget(const std::weak_ptr<UIBase>& target) {
  if (drawable_ != nullptr || target.expired()) {
    return;
  }
  drawable_ = std::make_unique<ImageDrawable>(target);
  ApplyPaintInfo();
  ApplyImage();
}

void LynxImageManager::UpdateBounds(float width, float height,
                                    float scale_density) {
  const float physical_width = width * scale_density;
  const float physical_height = height * scale_density;
  if (drawable_ == nullptr ||
      (has_bounds_ && base::FloatsEqual(width_, physical_width) &&
       base::FloatsEqual(height_, physical_height))) {
    return;
  }
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

void LynxImageManager::ApplyPaintInfo() {
  if (drawable_ == nullptr) {
    return;
  }
  drawable_->UpdateMode(ToImageDrawableMode(paint_info_.mode));
  drawable_->UpdateLoopCount(std::max(paint_info_.loop_count, 0));
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

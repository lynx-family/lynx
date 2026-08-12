// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"

#include <algorithm>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/value/table.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/image_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

ArkUI_ObjectFit ToArkUIObjectFit(ImageFitMode mode) {
  switch (mode) {
    case ImageFitMode::kAspectFit:
      return ARKUI_OBJECT_FIT_CONTAIN;
    case ImageFitMode::kAspectFill:
      return ARKUI_OBJECT_FIT_COVER;
    case ImageFitMode::kCenter:
      return ARKUI_OBJECT_FIT_NONE;
    case ImageFitMode::kScaleToFill:
      return ARKUI_OBJECT_FIT_FILL;
  }
  return ARKUI_OBJECT_FIT_FILL;
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

LynxImageManager::LynxImageManager(LynxContext* context) : context_(context) {}

LynxImageManager::~LynxImageManager() { Reset(); }

void LynxImageManager::UpdatePaintInfo(const ImagePaintInfo& paint_info) {
  paint_info_ = paint_info;
  ApplyPaintInfo();
}

void LynxImageManager::RequestImage(int32_t sign, std::string src, float width,
                                    float height, int32_t event_mask) {
  sign_ = sign;
  event_mask_ = event_mask;
  src_ = std::move(src);
  const uint64_t request_id = ++request_id_;
  if (src_.empty()) {
    return;
  }
  auto* image_service = UIOwner::image_service;
  if (image_service == nullptr) {
    SendErrorEvent(image::kPathErrorCode, image::kPathErrorMsg);
    return;
  }

  ImageRequestInfo request{
      .url = src_,
      .mode = ToArkUIObjectFit(paint_info_.mode),
  };
  image_service->DecodeImage(
      request,
      [weak_self = weak_from_this(),
       request_id](const std::shared_ptr<ImageData>& image) {
        auto self = weak_self.lock();
        if (!self || !self->context_) {
          return;
        }
        if (!self->IsCurrentRequest(request_id)) {
          return;
        }
        self->image_ = image;
        self->ApplyImage();
      },
      [weak_self = weak_from_this(), request_id](float image_width,
                                                 float image_height) {
        auto self = weak_self.lock();
        if (self) {
          self->OnImageLoadSuccess(request_id, image_width, image_height);
        }
      },
      [weak_self = weak_from_this(), request_id](
          int error_code, const std::string& error_message) {
        auto self = weak_self.lock();
        if (self) {
          self->OnImageLoadFailure(request_id, error_code, error_message);
        }
      });
}

void LynxImageManager::SetTarget(const std::weak_ptr<UIBase>& target) {
  auto current_target = target_.lock();
  auto new_target = target.lock();
  if (current_target == new_target && drawable_) {
    return;
  }

  target_ = target;
  drawable_.reset();
  has_bounds_ = false;
  if (!new_target) {
    return;
  }
  drawable_ = std::make_unique<ImageDrawable>([weak_target = target_] {
    auto target = weak_target.lock();
    if (target != nullptr) {
      target->Invalidate();
    }
  });
  ApplyPaintInfo();
  ApplyImage();
}

void LynxImageManager::UpdateBounds(float width, float height,
                                    float scale_density) {
  if (!drawable_ || (has_bounds_ && base::FloatsEqual(width_, width) &&
                     base::FloatsEqual(height_, height) &&
                     base::FloatsEqual(scale_density_, scale_density))) {
    return;
  }
  width_ = width;
  height_ = height;
  scale_density_ = scale_density;
  has_bounds_ = true;
  drawable_->UpdateBounds(0.f, 0.f, width, height, 0.f, 0.f, 0.f, 0.f,
                          scale_density);
}

void LynxImageManager::Draw(OH_Drawing_Canvas* canvas) {
  if (drawable_ && canvas) {
    drawable_->Render(canvas);
  }
}

void LynxImageManager::Reset() {
  ++request_id_;
  image_.reset();
  if (drawable_) {
    drawable_->StopAnimation();
  }
  drawable_.reset();
  target_.reset();
  has_bounds_ = false;
}

void LynxImageManager::ApplyPaintInfo() {
  if (!drawable_) {
    return;
  }
  drawable_->UpdateMode(ToImageDrawableMode(paint_info_.mode));
  drawable_->UpdateLoopCount(std::max(paint_info_.loop_count, 0));
}

void LynxImageManager::ApplyImage() {
  if (!drawable_ || !image_) {
    return;
  }
  drawable_->UpdateDrawCurrent(image_, /*prepare_draw_resources=*/true);
  if (paint_info_.autoplay) {
    drawable_->StartAnimation();
  }
}

void LynxImageManager::OnImageLoadSuccess(uint64_t request_id, float width,
                                          float height) {
  if (!IsCurrentRequest(request_id)) {
    return;
  }
  SendLoadEvent(width, height);
}

void LynxImageManager::OnImageLoadFailure(uint64_t request_id,
                                          int32_t error_code,
                                          const std::string& error_message) {
  if (!IsCurrentRequest(request_id)) {
    return;
  }
  SendErrorEvent(error_code, error_message);
}

void LynxImageManager::SendLoadEvent(float width, float height) const {
  if (context_ == nullptr || (event_mask_ & image::kFlagImageLoadEvent) == 0) {
    return;
  }
  auto detail = lepus::Dictionary::Create();
  detail->SetValue(image::kLoadEventImageWidth, width);
  detail->SetValue(image::kLoadEventImageHeight, height);
  CustomEvent event{sign_, image::kLoadEventName, "detail",
                    lepus_value(detail)};
  context_->SendEvent(event);
}

void LynxImageManager::SendErrorEvent(int32_t error_code,
                                      const std::string& error_message) const {
  if (context_ == nullptr || src_.empty() ||
      (event_mask_ & image::kFlagImageErrorEvent) == 0) {
    return;
  }
  auto detail = lepus::Dictionary::Create();
  detail->SetValue(image::kErrorEventCode, error_code);
  detail->SetValue(image::kErrorEventMsg, error_message);
  CustomEvent event{sign_, image::kErrorEventName, "detail",
                    lepus_value(detail)};
  context_->SendEvent(event);
}

bool LynxImageManager::IsCurrentRequest(uint64_t request_id) const {
  return request_id_ == request_id;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

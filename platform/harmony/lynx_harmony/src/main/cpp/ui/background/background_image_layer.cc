// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_image_layer.h"

#include <filemanagement/file_uri/oh_file_uri.h>
#include <native_drawing/drawing_pixel_map.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>

#include <algorithm>

#include "core/renderer/utils/lynx_env.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

static constexpr const char* const kBackgroundImageEventLoad = "bgload";
static constexpr const char* const kBackgroundImageEventError = "bgerror";

void BackgroundImageLayer::OnSizeChange(float width, float height,
                                        float scale_density) {
  BackgroundLayer::OnSizeChange(width, height, scale_density);
  if (dest_rect_) {
    OH_Drawing_RectDestroy(dest_rect_);
    dest_rect_ = nullptr;
  }
  dest_rect_ = OH_Drawing_RectCreate(0, 0, width_, height_);
}

void BackgroundImageLayer::Draw(OH_Drawing_Canvas* canvas,
                                OH_Drawing_Path* path) {
  if (!src_rect_ || !dest_rect_ || !sample_) {
    LOGE("Draw with invalid args.");
    return;
  }

  OH_Drawing_PixelMap* draw_bitmap = nullptr;
  if (image_drawable_) {
    draw_bitmap = image_drawable_->GetCurrentDrawBitmap();
  } else if (pixel_map_ && pixel_map_->FirstFrame() &&
             pixel_map_->FirstFrame()->DrawBitmap()) {
    draw_bitmap = pixel_map_->FirstFrame()->DrawBitmap();
  }

  if (!draw_bitmap) {
    LOGE("Draw with invalid args.");
    return;
  }
  OH_Drawing_CanvasDrawPixelMapRect(canvas, draw_bitmap, src_rect_, dest_rect_,
                                    sample_);
}

bool BackgroundImageLayer::IsReady() {
  return pixel_map_ != nullptr ||
         (image_drawable_ && image_drawable_->HasContent());
}

void BackgroundImageLayer::OnUpdateBounds() {
  if (image_loaded) {
    return;
  }
  LoadImage();
  image_loaded = true;
}

float BackgroundImageLayer::GetWidth() { return image_width_; }

float BackgroundImageLayer::GetHeight() { return image_height_; }

BackgroundImageLayer::~BackgroundImageLayer() {
  DestroyDrawStruct();
  if (dest_rect_) {
    OH_Drawing_RectDestroy(dest_rect_);
    dest_rect_ = nullptr;
  }
}

void BackgroundImageLayer::LoadImage() {
  auto ui_base_self = ui_base_.lock();
  if (!ui_base_self) {
    return;
  }
  static bool enable_new_image = LynxEnv::GetInstance().EnableHarmonyNewImage();
  if (UIOwner::image_service && enable_new_image) {
    LoadImageFromService();
    return;
  }
  auto request = pub::LynxResourceRequest{url_, pub::LynxResourceType::kImage};
  if (url_.find("http") != std::string::npos) {
    auto resource_loader = ui_base_self->GetContext()->GetResourceLoader();
    if (!resource_loader) {
      return;
    }
    resource_loader->LoadResourcePath(
        request,
        [weak_self = weak_from_this()](pub::LynxPathResponse& response) {
          if (response.Success()) {
            auto self = weak_self.lock();
            if (!self) {
              return;
            }
            auto image_layer =
                std::static_pointer_cast<BackgroundImageLayer>(self);
            image_layer->DecodeImageData(response.path, false);
          }
        });
  } else {
    bool is_base64 = base::BeginsWith(url_, image::kBase64Scheme);
    DecodeImageData(url_, is_base64);
  }
}

void BackgroundImageLayer::LoadImageFromService() {
  auto ui_base_self = ui_base_.lock();
  if (!ui_base_self) {
    return;
  }
  std::string final_url;
  if (ui_base_self->SkipRedirection()) {
    final_url = url_;
  } else {
    final_url = LynxImageHelper::GetRedirectUrl(
        url_, ui_base_self->GetContext()->GetResourceLoader());
  }
  ImageRequestInfo request;
  request.url = final_url;
  UIOwner::image_service->DecodeImage(
      request,
      [weak_self = weak_from_this()](const std::shared_ptr<ImageData>& data) {
        auto self = weak_self.lock();
        if (!self || !data) {
          return;
        }
        auto image_layer = std::static_pointer_cast<BackgroundImageLayer>(self);
        auto ui_base_self = image_layer->ui_base_.lock();
        if (!ui_base_self || !data->Pixelmap()) {
          return;
        }
        image_layer->DestroyDrawStruct();
        bool pixelated = ui_base_self->RenderingType() ==
                         starlight::ImageRenderingType::kPixelated;
        image_layer->sample_ = OH_Drawing_SamplingOptionsCreate(
            pixelated ? FILTER_MODE_NEAREST : FILTER_MODE_LINEAR,
            pixelated ? MIPMAP_MODE_NEAREST : MIPMAP_MODE_LINEAR);
        OH_Pixelmap_ImageInfo* pixel_map_info = nullptr;
        OH_PixelmapImageInfo_Create(&pixel_map_info);
        OH_PixelmapNative_GetImageInfo(data->Pixelmap(), pixel_map_info);
        OH_PixelmapImageInfo_GetWidth(pixel_map_info,
                                      &image_layer->image_width_);
        OH_PixelmapImageInfo_GetHeight(pixel_map_info,
                                       &image_layer->image_height_);
        OH_PixelmapImageInfo_Release(pixel_map_info);
        image_layer->src_rect_ = OH_Drawing_RectCreate(
            0, 0, image_layer->image_width_, image_layer->image_height_);
        image_layer->image_drawable_ =
            std::make_unique<ImageDrawable>(image_layer->ui_base_);
        image_layer->image_drawable_->UpdateLoopCount(0);
        image_layer->image_drawable_->UpdateDrawCurrent(data);
        image_layer->image_drawable_->StartAnimation();
        image_layer->pixel_map_.reset();
        ui_base_self->Invalidate();
      },
      [weak_self = weak_from_this(), url = url_](float width,
                                                 float height) mutable {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        auto image_layer = std::static_pointer_cast<BackgroundImageLayer>(self);
        auto ui_base_self = image_layer->ui_base_.lock();
        if (!ui_base_self || !ui_base_self->GetContext()) {
          return;
        }
        auto events = ui_base_self->EventSet();
        if (std::find(events.begin(), events.end(),
                      kBackgroundImageEventLoad) == events.end()) {
          return;
        }
        auto dict = lepus::Dictionary::Create();
        dict->SetValue("height", height);
        dict->SetValue("width", width);
        dict->SetValue("url", url);
        CustomEvent event{ui_base_self->Sign(), kBackgroundImageEventLoad,
                          "detail", lepus_value(dict)};
        ui_base_self->GetContext()->SendEvent(event);
      },
      [weak_self = weak_from_this(), url = url_](
          int error_code, const std::string& error_msg) mutable {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        auto image_layer = std::static_pointer_cast<BackgroundImageLayer>(self);
        auto ui_base_self = image_layer->ui_base_.lock();
        if (!ui_base_self || !ui_base_self->GetContext()) {
          return;
        }
        auto events = ui_base_self->EventSet();
        if (std::find(events.begin(), events.end(),
                      kBackgroundImageEventError) == events.end()) {
          return;
        }
        auto dict = lepus::Dictionary::Create();
        dict->SetValue("error_code", error_code);
        dict->SetValue("errMsg", error_msg);
        dict->SetValue("url", url);
        CustomEvent event{ui_base_self->Sign(), kBackgroundImageEventError,
                          "detail", lepus_value(dict)};
        ui_base_self->GetContext()->SendEvent(event);
      });
}

void BackgroundImageLayer::DecodeImageData(const std::string& url,
                                           bool is_base64) {
  auto ui_base_self = ui_base_.lock();
  if (!ui_base_self) {
    return;
  }
  auto env = ui_base_self->GetContext()->GetNapiEnv();
  if (!env) {
    return;
  }
  LynxImageHelper::DecodeImageAsync(
      env, url, is_base64,
      [weak_self = weak_from_this()](LynxImageHelper::ImageResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        auto image_layer = std::static_pointer_cast<BackgroundImageLayer>(self);
        image_layer->HandleImageData(response);
      });
}

void BackgroundImageLayer::DestroyDrawStruct() {
  image_drawable_.reset();
  if (src_rect_) {
    OH_Drawing_RectDestroy(src_rect_);
    src_rect_ = nullptr;
  }
  if (sample_) {
    OH_Drawing_SamplingOptionsDestroy(sample_);
    sample_ = nullptr;
  }
}

void BackgroundImageLayer::HandleImageData(
    LynxImageHelper::ImageResponse& response) {
  if (!response.Success()) {
    LOGE("decode error " << response.err_code);
    return;
  }
  auto ui_base_self = ui_base_.lock();
  if (!ui_base_self) {
    return;
  }
  DestroyDrawStruct();
  bool pixelated = ui_base_self->RenderingType() ==
                   starlight::ImageRenderingType::kPixelated;
  sample_ = OH_Drawing_SamplingOptionsCreate(
      pixelated ? FILTER_MODE_NEAREST : FILTER_MODE_LINEAR,
      pixelated ? MIPMAP_MODE_NEAREST : MIPMAP_MODE_LINEAR);
  pixel_map_ = std::move(response.data);
  image_width_ = pixel_map_->Width();
  image_height_ = pixel_map_->Height();
  src_rect_ = OH_Drawing_RectCreate(0, 0, image_width_, image_height_);
  ui_base_self->Invalidate();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

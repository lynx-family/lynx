// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_image_layer.h"

#include <native_drawing/drawing_pixel_map.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

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
  if (!draw_pixel_map_ || !src_rect_ || !dest_rect_) {
    return;
  }
  OH_Drawing_CanvasDrawPixelMapRect(canvas, draw_pixel_map_, src_rect_,
                                    dest_rect_, sample_);
}

bool BackgroundImageLayer::IsReady() { return pixel_map_ != nullptr; }

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
    DecodeImageData(url_, true);
  }
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
  if (src_rect_) {
    OH_Drawing_RectDestroy(src_rect_);
    src_rect_ = nullptr;
  }
  if (sample_) {
    OH_Drawing_SamplingOptionsDestroy(sample_);
    sample_ = nullptr;
  }
  if (draw_pixel_map_) {
    OH_Drawing_PixelMapDissolve(draw_pixel_map_);
    draw_pixel_map_ = nullptr;
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
  image_width_ = response.image_width;
  image_height_ = response.image_height;
  src_rect_ = OH_Drawing_RectCreate(0, 0, image_width_, image_height_);
  draw_pixel_map_ =
      OH_Drawing_PixelMapGetFromOhPixelMapNative(pixel_map_.get());
  ui_base_self->Invalidate();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

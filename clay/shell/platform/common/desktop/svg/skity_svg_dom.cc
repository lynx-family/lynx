// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/svg/skity_svg_dom.h"

#include <utility>

#include "clay/shell/platform/common/desktop/svg/skity_svg_canvas_delegate.h"
#include "parser/SrSVGDOM.h"
#include "platform/skity/SrSkityCanvas.h"

namespace clay {

std::shared_ptr<SVGDom> SVGDom::Create(std::shared_ptr<skity::Data> data,
                                       SVGDom::ImageCallback callback) {
  return std::make_shared<SkitySVGDom>(data, std::move(callback));
}

SkitySVGDom::SkitySVGDom(std::shared_ptr<skity::Data> data,
                         SVGDom::ImageCallback callback)
    : data_(data), callback_(std::move(callback)) {
  if (data_ && data_->Size() > 0) {
    svg_dom_ = serval::svg::parser::SrSVGDOM::make((const char*)data_->Bytes(),
                                                   data_->Size(), nullptr);
  }
}

SkitySVGDom::~SkitySVGDom() = default;

std::shared_ptr<skity::Image> SkitySVGDom::Render(
    int width, int height, fml::RefPtr<GPUUnrefQueue> unref_queue) {
  if (!unref_queue || !unref_queue->GetContext()) {
    return nullptr;
  }

  if (!svg_dom_) {
    return nullptr;
  }

  auto image = skity::Image::MakeDeferredTextureImage(
      skity::TextureFormat::kRGBA, width, height,
      skity::AlphaType::kPremul_AlphaType);

  unref_queue->GetTaskRunner()->PostTask([image, width, height,
                                          context = unref_queue->GetContext(),
                                          weak = weak_from_this()]() {
    if (auto self = weak.lock()) {
      SkitySVGDom* dom = static_cast<SkitySVGDom*>(self.get());
      skity::PictureRecorder recorder;
      recorder.BeginRecording(skity::Rect::MakeWH(width, height));
      auto* recording_canvas = recorder.GetRecordingCanvas();
      if (!recording_canvas) {
        return;
      }
      auto delegate =
          std::make_unique<SkitySVGCanvasDelegate>(recording_canvas);
      serval::svg::skity::SrSkityCanvas sr_canvas(delegate.get(),
                                                  dom->callback_);
      SrSVGBox view_port{0.f, 0.f, (float)width, (float)height};
      dom->svg_dom_->Render(&sr_canvas, view_port);
      auto dl = recorder.FinishRecording();
      if (!dl) {
        return;
      }
      if (!delegate->CanUseSoftwareCanvas()) {
        skity::GPURenderTargetDescriptor desc;
        desc.width = width;
        desc.height = height;
        desc.sample_count = 4;
        auto render_target = context->CreateRenderTarget(desc);
        dl->Draw(render_target->GetCanvas());
        auto snapshot = context->MakeSnapshot(std::move(render_target));
        if (snapshot) {
          image->SetTexture(*snapshot->GetTexture());
        }
      } else {
        auto bitmap = std::make_unique<skity::Bitmap>(
            width, height, skity::AlphaType::kPremul_AlphaType);
        auto canvas = skity::Canvas::MakeSoftwareCanvas(bitmap.get());
        dl->Draw(canvas.get());
        auto pixmap = bitmap->GetPixmap();
        if (pixmap) {
          auto texture = context->CreateTexture(
              skity::Texture::FormatFromColorType(pixmap->GetColorType()),
              pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
          texture->DeferredUploadImage(std::move(pixmap));
          image->SetTexture(texture);
        }
      }
    }
  });

  return image;
}

}  // namespace clay

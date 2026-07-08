// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/image/svg_image.h"

#include "base/include/md5.h"
#include "clay/fml/logging.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/image/base_image_instance.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/ui/resource/image_fetcher.h"

namespace clay {
std::shared_ptr<SVGImage> SVGImage::Make(
    fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
    const std::string& content) {
  auto image = std::shared_ptr<SVGImage>(new SVGImage(image_fetcher, content));
  image->type_ = ImageType::kSVG;
  image->url_ = std::move(url);
  image->InitializeSVGDom();
  return image;
}

SVGImage::SVGImage(fml::WeakPtr<ImageFetcher> image_fetcher,
                   const std::string& content)
    : content_(content) {
  image_fetcher_ = std::move(image_fetcher);
}

void SVGImage::InitializeSVGDom() {
  std::weak_ptr<BaseImage> weak_self = weak_from_this();
  svg_dom_ = SVGDom::Create(
      GrData::MakeWithProc(content_.data(), content_.size(), nullptr, nullptr),
      [weak_self](std::string url) -> std::shared_ptr<skity::Image> {
        auto self = weak_self.lock();
        if (!self) {
          return nullptr;
        }
        return static_cast<SVGImage*>(self.get())->LoadExternalImage(url);
      });
}

std::shared_ptr<skity::Image> SVGImage::LoadExternalImage(
    const std::string& url) {
  if (url.empty() || !image_fetcher_) {
    return nullptr;
  }
  return image_fetcher_->LoadImage(url);
}

void SVGImage::ScheduleRepaintAfterUpload(
    fml::RefPtr<GPUUnrefQueue> unref_queue) {
  if (!image_fetcher_ || !unref_queue) {
    return;
  }

  auto gpu_task_runner = unref_queue->GetTaskRunner();
  auto ui_task_runner = image_fetcher_->GetUITaskRunner();
  if (!gpu_task_runner || !ui_task_runner) {
    return;
  }

  // SVG rendering fills the deferred image on the GPU task runner after the
  // current paint pass. Request one more UI frame after that task has run.
  std::weak_ptr<BaseImage> weak_self = weak_from_this();
  gpu_task_runner->PostTask([weak_self, ui_task_runner]() {
    ui_task_runner->PostTask([weak_self]() {
      auto self = weak_self.lock();
      if (!self) {
        return;
      }
      static_cast<SVGImage*>(self.get())->NotifyImageChanged();
    });
  });
}

void SVGImage::NotifyImageChanged() {
  for (auto* instance : instances_) {
    instance->OnNotifyAnimationFrame();
  }
}

void SVGImage::Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) {
  if (!unref_queue || !unref_queue->GetContext()) {
    FML_LOG(ERROR) << "SVGImage::Upload: unref_queue or context is null";
    return;
  }
  if (!svg_dom_) {
    FML_LOG(ERROR) << "SVGImage::Upload: svg dom is null";
    return;
  }
  if (!gpu_image_.object() || gpu_image_.object()->width() < size.width() ||
      gpu_image_.object()->height() < size.height()) {
    auto image = svg_dom_->Render(size.width(), size.height(), unref_queue);
    if (!image) {
      FML_LOG(ERROR) << "SVGImage::Paint: " << size.width() << " "
                     << size.height();
      return;
    }
    gpu_image_ = GPUObject(GraphicsImage::Make(image), unref_queue);
    ScheduleRepaintAfterUpload(unref_queue);
  }
}
}  // namespace clay

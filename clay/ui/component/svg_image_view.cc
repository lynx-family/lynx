// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/svg_image_view.h"

#include <memory>
#include <utility>

#include "clay/gfx/image/image.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/resource/image_manager.h"

namespace clay {

SVGImageView::SVGImageView(int id, PageView* page_view)
    : BaseImageView(id, "svg", std::make_unique<RenderImage>(), page_view),
      weak_factory_(this) {}

void SVGImageView::DidUpdateAttributes() {
  NotifyLoadIfNeeded();
  BaseImageView::DidUpdateAttributes();
}

void SVGImageView::NotifyLoadIfNeeded() {
  if (!pending_load_event_) {
    return;
  }

  auto image_instance = GetRenderImage()->GetImage();
  if (!image_instance) {
    return;
  }

#ifdef ENABLE_SKITY
  auto graphics_image = image_instance->GetGraphicsImage();
  if (!graphics_image) {
    return;
  }
  auto skity_image = graphics_image->gr_image();
  if (!skity_image ||
      (skity_image->IsTextureBackend() && !skity_image->IsLazy() &&
       skity_image->GetTexture() == nullptr)) {
    return;
  }
#endif  // ENABLE_SKITY

  pending_load_event_ = false;
  NotifyLoadSuccess(image_instance->GetImage()->GetWidth(),
                    image_instance->GetImage()->GetHeight());
}

void SVGImageView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kContent) {
    SetContent(attribute_utils::GetCString(value));
  } else {
    BaseImageView::SetAttribute(attr_c, value);
  }
}

void SVGImageView::SetContent(const std::string content) {
  if (content.empty()) {
    return;
  }
  if (content.compare(content_) == 0) {
    return;
  }

  if (!content_.empty()) {
    content_.clear();
    GetRenderImage()->SetImage(nullptr);
  }

  // We assume this svg may change multiple times, so this view deserves
  // to be a repaint boundary.
  if (++update_counter_ >= 3) {
    SetRepaintBoundary(true);
  }

  content_ = std::move(content);
#ifndef ENABLE_SKITY
  auto image_resource =
      page_view_->GetImageResourceFetcher()->GetImageResourceFromSVGContent(
          content_, page_view_->UseTextureBackend(),
          page_view_->DeferredImageDecode(),
          page_view_->ImageDecodeWithPriority());
  GetRenderImage()->SetImage(std::move(image_resource));
#else
  page_view_->GetImageResourceFetcher()->FetchSVGImageWithContent(
      content_, [self = weak_factory_.GetWeakPtr()](
                    std::unique_ptr<BaseImageInstance> image, bool hit_cache) {
        if (!self) {
          return;
        }
        if (image) {
          image->SetAnimationFrameCallback([self]() {
            if (!self) {
              return;
            }
            self->Invalidate();
            self->NotifyLoadIfNeeded();
          });
        }
        static_cast<RenderImage*>(self->render_object())
            ->SetImage(std::move(image));
      });
#endif  // ENABLE_SKITY
  // // We delay the load event to ensure the events have been added.
  pending_load_event_ = true;
  Invalidate();
}

#ifndef NDEBUG
std::string SVGImageView::ToString() const {
  std::stringstream ss;
  ss << BaseImageView::ToString();
  ss << " content_=(" << content_ << ")";
  return ss.str();
}
#endif

}  // namespace clay

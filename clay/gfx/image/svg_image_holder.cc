// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/svg_image_holder.h"

#include <algorithm>
#include <future>
#include <string>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "clay/fml/logging.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {
SVGImageHolder::SVGImageHolder()
    : status_(SVGStatus::kNew), mutex_(fml::SharedMutex::Create()) {}

SVGImageHolder::~SVGImageHolder() {
  {
    fml::UniqueLock lock(*mutex_);
    if (status_ == SVGStatus::kInProgress) {
      svg_dom_promise_.set_value(nullptr);
    }
  }
}

SVGDomPtr SVGImageHolder::GetSVGDOM() {
  {
    fml::UniqueLock lock(*mutex_);
    if (svg_dom_) {
      return svg_dom_;
    }
  }
  if (svg_dom_future_.valid()) {
    svg_dom_ = svg_dom_future_.get();
  }
  return svg_dom_;
}

void SVGImageHolder::CreateSVGDOM(GrDataPtr data) {
  FML_CHECK(data);
  {
    fml::UniqueLock lock(*mutex_);
    if (status_ == SVGStatus::kComplete || status_ == SVGStatus::kInProgress) {
      return;
    }

    status_ = SVGStatus::kInProgress;
  }

  fml::RefPtr<SVGImageHolder>* raw_holder_ref =
      new fml::RefPtr<SVGImageHolder>(this);

  GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
      fml::MakeCopyable([raw_holder_ref, data]() {
        TRACE_EVENT("clay", "CreateSVGDOM");
        std::unique_ptr<fml::RefPtr<SVGImageHolder> > holder_ref(
            raw_holder_ref);
        fml::RefPtr<SVGImageHolder> holder(std::move(*holder_ref));
        {
          fml::UniqueLock lock(*holder->mutex_);

#ifndef ENABLE_SKITY
          std::unique_ptr<SkStream> stream = SkMemoryStream::Make(data);
          if (stream) {
            holder->svg_dom_promise_.set_value(
                SkSVGDOM::MakeFromStream(*stream));
          } else {
            FML_LOG(ERROR) << "Invalid svg data.";
            holder->svg_dom_promise_.set_value(nullptr);
          }
#else
          if (data) {
            holder->svg_dom_promise_.set_value(SVGDom::Create(data));
          } else {
            FML_LOG(ERROR) << "Invalid svg data.";
            holder->svg_dom_promise_.set_value(nullptr);
          }
#endif  // ENABLE_SKITY

          holder->status_ = SVGStatus::kComplete;
        }
      }));
}

fml::RefPtr<GraphicsImage> SVGImageHolder::GetGraphicsImage() const {
  return svg_image_wrapper_ ? svg_image_wrapper_->GetGraphicsImage() : nullptr;
}

void SVGImageHolder::SetGraphicsImage(
    fml::RefPtr<GraphicsImageWrapper> image_wrapper) {
  svg_image_wrapper_ = image_wrapper;
}
}  // namespace clay

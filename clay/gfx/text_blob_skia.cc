// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/text_blob_skia.h"

#include <utility>

#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {

TextBlobSkia::TextBlobSkia(sk_sp<SkTextBlob> text_blob)
    : text_blob_(std::move(text_blob)) {}

// |TextBlob|
TextBlobSkia::~TextBlobSkia() = default;

// |TextBlob|
sk_sp<SkTextBlob> TextBlobSkia::gr_text_blob() const { return text_blob_; };

// |TextBlob|
skity::Rect TextBlobSkia::bounds() const {
  if (!text_blob_) {
    return skity::Rect::MakeEmpty();
  }
  return ConvertSkRectToSkityRect(text_blob_->bounds());
}

}  // namespace clay

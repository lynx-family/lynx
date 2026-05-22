// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/text_blob_skity.h"

#include <utility>

namespace clay {

fml::RefPtr<TextBlobSkity> TextBlobSkity::Make(
    std::shared_ptr<skity::TextBlob> text_blob) {
  if (!text_blob) {
    return nullptr;
  }
  return fml::MakeRefCounted<TextBlobSkity>(std::move(text_blob));
}

TextBlobSkity::TextBlobSkity(std::shared_ptr<skity::TextBlob> text_blob)
    : text_blob_(std::move(text_blob)) {}

// |TextBlob|
TextBlobSkity::~TextBlobSkity() = default;

// |TextBlob|
std::shared_ptr<skity::TextBlob> TextBlobSkity::gr_text_blob() const {
  return text_blob_;
}

// |TextBlob|
skity::Rect TextBlobSkity::bounds() const {
  if (!text_blob_) {
    return skity::Rect::MakeEmpty();
  }

  auto bound_size = text_blob_->GetBoundSize();
  return skity::Rect::MakeSize({bound_size.x, bound_size.y});
}

}  // namespace clay

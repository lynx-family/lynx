// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/text_blob.h"

#include <utility>

#include "clay/gfx/gfx_rendering_backend.h"

namespace clay {

#ifndef ENABLE_SKITY
fml::RefPtr<TextBlob> TextBlob::Make(const SkTextBlob* text_blob) {
  return Make(sk_ref_sp(text_blob));
}

fml::RefPtr<TextBlob> TextBlob::Make(sk_sp<SkTextBlob> text_blob) {
  return fml::MakeRefCounted<TextBlobSkia>(std::move(text_blob));
}
#else
fml::RefPtr<TextBlob> TextBlob::Make(
    std::shared_ptr<skity::TextBlob> text_blob) {
  return fml::MakeRefCounted<TextBlobSkity>(std::move(text_blob));
}
#endif

TextBlob::TextBlob() = default;

TextBlob::~TextBlob() = default;

}  // namespace clay

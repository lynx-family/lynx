// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_TEXT_BLOB_SKIA_H_
#define CLAY_GFX_TEXT_BLOB_SKIA_H_

#include <memory>

#include "clay/gfx/text_blob.h"
#include "third_party/skia/include/core/SkTextBlob.h"

namespace clay {

class TextBlobSkia final : public TextBlob {
 public:
  explicit TextBlobSkia(sk_sp<SkTextBlob> text_blob);

  // |TextBlob|
  ~TextBlobSkia() override;

  // |TextBlob|
  sk_sp<SkTextBlob> gr_text_blob() const override;

  // |TextBlob|
  skity::Rect bounds() const override;

 private:
  sk_sp<SkTextBlob> text_blob_;

  BASE_DISALLOW_COPY_AND_ASSIGN(TextBlobSkia);
};

}  // namespace clay

#endif  // CLAY_GFX_TEXT_BLOB_SKIA_H_

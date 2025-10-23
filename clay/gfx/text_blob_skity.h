// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_TEXT_BLOB_SKITY_H_
#define CLAY_GFX_TEXT_BLOB_SKITY_H_

#include <memory>

#include "clay/gfx/text_blob.h"
#include "skity/text/text_blob.hpp"

namespace clay {

class TextBlobSkity final : public TextBlob {
 public:
  static fml::RefPtr<TextBlobSkity> Make(
      std::shared_ptr<skity::TextBlob> text_blob);

  explicit TextBlobSkity(std::shared_ptr<skity::TextBlob> text_blob);

  // |TextBlob|
  ~TextBlobSkity() override;

  // |TextBlob|
  std::shared_ptr<skity::TextBlob> gr_text_blob() const override;

  // |TextBlob|
  skity::Rect bounds() const override;

 private:
  std::shared_ptr<skity::TextBlob> text_blob_;

  BASE_DISALLOW_COPY_AND_ASSIGN(TextBlobSkity);
};

}  // namespace clay

#endif  // CLAY_GFX_TEXT_BLOB_SKITY_H_

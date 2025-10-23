// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_TEXT_BLOB_H_
#define CLAY_GFX_TEXT_BLOB_H_

#include <memory>
#include <optional>
#include <string>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

namespace skity {
class TextBlob;
}  // namespace skity

namespace clay {

//------------------------------------------------------------------------------
/// @brief      TextBlob combines multiple text runs into an immutable
///             container.
///
class TextBlob : public fml::RefCountedThreadSafe<TextBlob> {
 public:
#ifndef ENABLE_SKITY
  static fml::RefPtr<TextBlob> Make(const SkTextBlob* text_blob);
#endif  // ENABLE_SKITY

  static fml::RefPtr<TextBlob> Make(GrTextBlobPtr text_blob);

  virtual ~TextBlob();

  virtual GrTextBlobPtr gr_text_blob() const = 0;

  virtual skity::Rect bounds() const = 0;

 protected:
  TextBlob();
};

}  // namespace clay

#endif  // CLAY_GFX_TEXT_BLOB_H_

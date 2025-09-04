// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_BACKGROUND_DATA_H_
#define CORE_STYLE_BACKGROUND_DATA_H_

#include <array>
#include <vector>

#include "base/include/flex_optional.h"
#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/style/color.h"
#include "core/style/default_computed_style.h"

namespace lynx {
namespace starlight {

struct BackgroundData {
  struct BackgroundImageData {
    uint32_t image_count{DefaultComputedStyle::DEFAULT_LONG};
    bool clone_image = false;
    lepus::Value image;

    base::InlineVector<NLength, 1> position;
    base::InlineVector<NLength, 1> size;
    base::InlineVector<BackgroundRepeatType, 1> repeat;
    base::InlineVector<BackgroundOriginType, 1> origin;
    base::InlineVector<BackgroundClipType, 1> clip;
  };

  BackgroundData() = default;
  ~BackgroundData() = default;

  uint32_t color{DefaultColor::DEFAULT_COLOR};
  base::flex_optional<BackgroundImageData> image_data;

  // A flag telling `base::flex_optional<>` to save memory.
  using AlwaysUseFlexOptionalMemSave = bool;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_BACKGROUND_DATA_H_

// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_BACKGROUND_DATA_H_
#define CORE_STYLE_BACKGROUND_DATA_H_

#include <array>
#include <vector>

#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace starlight {

struct BackgroundData {
  BackgroundData();
  ~BackgroundData() = default;
  unsigned int color;
  unsigned int image_count;
  lepus::Value image;
  std::vector<NLength> position;
  std::vector<NLength> size;
  std::vector<BackgroundRepeatType> repeat;
  std::vector<BackgroundOriginType> origin;
  std::vector<BackgroundClipType> clip;
  bool HasBackground() const;
  bool operator==(const BackgroundData& rhs) const {
    return std::tie(color, image, image_count, position, size, repeat, origin,
                    clip) == std::tie(rhs.color, rhs.image, rhs.image_count,
                                      rhs.position, rhs.size, rhs.repeat,
                                      rhs.origin, rhs.clip);
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_BACKGROUND_DATA_H_

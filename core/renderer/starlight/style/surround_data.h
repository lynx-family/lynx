// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_SURROUND_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_SURROUND_DATA_H_

#include "core/renderer/starlight/style/borders_data.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {

class SurroundData {
 public:
  SurroundData();
  ~SurroundData() = default;
  void Reset();
  NLength left_;
  NLength right_;
  NLength top_;
  NLength bottom_;

  NLength margin_left_;
  NLength margin_right_;
  NLength margin_top_;
  NLength margin_bottom_;

  NLength padding_left_;
  NLength padding_right_;
  NLength padding_top_;
  NLength padding_bottom_;

  std::optional<BordersData> border_data_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_SURROUND_DATA_H_

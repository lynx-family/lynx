// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_STYLE_COLOR_H_
#define CORE_STYLE_COLOR_H_

namespace lynx {
namespace starlight {

// TODO(zhangxiao.ninja): this color class will be a common
// struct, the CSSColor in renderer/css/css_color.h just use for
// css module to parser css color description, Color class will be
// the result of CSSColor
class Color {
 public:
  static constexpr unsigned int Black = 0xFF000000;
  static constexpr unsigned int White = 0xFFFFFFFF;
  static constexpr unsigned int Gray = 0xFF808080;
  static constexpr unsigned int Transparent = 0x00000000;
};

struct DefaultColor {
  static constexpr unsigned int DEFAULT_COLOR = Color::Transparent;
  static constexpr unsigned int DEFAULT_BORDER_COLOR = Color::Black;
  static constexpr unsigned int DEFAULT_OUTLINE_COLOR = Color::Black;
  static constexpr unsigned int DEFAULT_SHADOW_COLOR = Color::Black;
  static constexpr unsigned int DEFAULT_TEXT_COLOR = Color::Black;
  static constexpr unsigned int DEFAULT_TEXT_BACKGROUND_COLOR = Color::White;
};

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_STYLE_COLOR_H_

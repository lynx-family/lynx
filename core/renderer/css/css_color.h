// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_COLOR_H_
#define CORE_RENDERER_CSS_CSS_COLOR_H_

#include <string>

namespace lynx {
namespace tasm {
enum class TokenType;

class CSSColor {
 public:
  CSSColor() : r_(0), g_(0), b_(0), a_(1.0f) {}
  CSSColor(unsigned char r, unsigned char g, unsigned char b, float a)
      : r_(r), g_(g), b_(b), a_(a) {}
  static bool Parse(const std::string& color_str, CSSColor& color);
  static bool ParseNamedColor(const std::string& color_str, CSSColor& color);
  static CSSColor CreateFromHSLA(float h, float s, float l, float a);
  static CSSColor CreateFromRGBA(float r, float g, float b, float a);
  static CSSColor CreateFromKeyword(TokenType);
  static bool IsColorIdentifier(TokenType type);

  unsigned int Cast() const;

  bool operator==(const CSSColor& other) const {
    return r_ == other.r_ && g_ == other.g_ && b_ == other.b_ && a_ == other.a_;
  }

  static constexpr unsigned int Black = 0xFF000000;
  static constexpr unsigned int White = 0xFFFFFFFF;
  static constexpr unsigned int Gray = 0xFF808080;
  static constexpr unsigned int Transparent = 0x00000000;

  unsigned char r_;
  unsigned char g_;
  unsigned char b_;
  float a_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_COLOR_H_

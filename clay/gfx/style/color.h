// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_COLOR_H_
#define CLAY_GFX_STYLE_COLOR_H_

#include <cmath>
#include <string>

#include "clay/gfx/rendering_backend.h"
namespace clay {
struct Color {
 public:
  constexpr Color() : argb(0xFF000000) {}
  constexpr Color(uint32_t argb) : argb(argb) {}

  static uint8_t toAlpha(float opacity) { return toC(opacity); }
  static constexpr float toOpacity(uint8_t alpha) { return toF(alpha); }

  static constexpr Color ARGBColor(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return Color(((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) |
                 ((b & 0xFF) << 0));
  }
  static constexpr Color RGBOColor(uint8_t r, uint8_t g, uint8_t b,
                                   float opacity) {
    return Color((static_cast<uint8_t>(opacity * 0xff) & 0xFF) << 24 |
                 ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF) << 0));
  }

  static constexpr Color ColorSetA(Color color, uint8_t a) {
    return Color((a & 0xFF) << 24 | (color.argb & 0x00FFFFFF));
  }

  // clang-format off
  static constexpr Color kTransparent()        {return 0x00000000;}
  static constexpr Color kBlack()              {return 0xFF000000;}
  static constexpr Color kWhite()              {return 0xFFFFFFFF;}
  static constexpr Color kRed()                {return 0xFFFF0000;}
  static constexpr Color kGreen()              {return 0xFF00FF00;}
  static constexpr Color kBlue()               {return 0xFF0000FF;}
  static constexpr Color kCyan()               {return 0xFF00FFFF;}
  static constexpr Color kMagenta()            {return 0xFFFF00FF;}
  static constexpr Color kYellow()             {return 0xFFFFFF00;}
  static constexpr Color kDarkGrey()           {return 0xFF3F3F3F;}
  static constexpr Color kMidGrey()            {return 0xFF808080;}
  static constexpr Color kLightGrey()          {return 0xFFC0C0C0;}
  // clang-format on

  static Color Lerp(Color a, Color b, float t);
  static Color AlphaBlend(Color foreground, Color background);

  static bool Parse(const std::string& color_str, Color* color);

  uint32_t argb;

  bool Opaque() const { return Alpha() == 0xFF; }

  int Alpha() const { return argb >> 24; }
  float Opacity() const { return AlphaF(); }
  int Red() const { return (argb >> 16) & 0xFF; }
  int Green() const { return (argb >> 8) & 0xFF; }
  int Blue() const { return argb & 0xFF; }

  float AlphaF() const { return toF(Alpha()); }
  float RedF() const { return toF(Red()); }
  float GreenF() const { return toF(Green()); }
  float BlueF() const { return toF(Blue()); }

  uint32_t premultipliedArgb() const {
    if (Opaque()) {
      return argb;
    }
    float f = AlphaF();
    return (argb & 0xFF000000) |     //
           toC(RedF() * f) << 16 |   //
           toC(GreenF() * f) << 8 |  //
           toC(BlueF() * f);
  }

  Color withAlpha(uint8_t alpha) const {  //
    return (argb & 0x00FFFFFF) | (alpha << 24);
  }
  Color withRed(uint8_t red) const {  //
    return (argb & 0xFF00FFFF) | (red << 16);
  }
  Color withGreen(uint8_t green) const {  //
    return (argb & 0xFFFF00FF) | (green << 8);
  }
  Color withBlue(uint8_t blue) const {  //
    return (argb & 0xFFFFFF00) | (blue << 0);
  }

  void GetARGB(float* a, float* r, float* g, float* b) const;
  Color Light() const;
  Color Dark() const;

  uint32_t Value() const { return argb; }

  Color modulateOpacity(float opacity) const {
    return opacity <= 0   ? withAlpha(0)
           : opacity >= 1 ? *this
                          : withAlpha(round(Alpha() * opacity));
  }

  operator uint32_t() const { return argb; }
  bool operator==(Color const& other) const { return argb == other.argb; }
  bool operator!=(Color const& other) const { return argb != other.argb; }
  bool operator==(uint32_t const& other) const { return argb == other; }
  bool operator!=(uint32_t const& other) const { return argb != other; }
#ifndef NDEBUG
  std::string ToString() const;
#endif

 private:
  static constexpr float toF(uint8_t comp) { return comp * (1.0 / 255); }
  static uint8_t toC(float fComp) { return round(fComp * 255); }
};

#ifndef ENABLE_SKITY
inline SkColor ToSk(const Color& color) {
  return SkColorSetARGB(color.Alpha(), color.Red(), color.Green(),
                        color.Blue());
}
#else
inline skity::Color ToSk(const Color& color) {
  return skity::ColorSetARGB(color.Alpha(), color.Red(), color.Green(),
                             color.Blue());
}
#endif  // ENABLE_SKITY
}  // namespace clay

#endif  // CLAY_GFX_STYLE_COLOR_H_

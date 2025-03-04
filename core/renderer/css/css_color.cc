// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_color.h"

#include <ctype.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "core/renderer/css/css_keywords.h"

namespace lynx {
namespace tasm {

#define ARRAY_SIZE(a) static_cast<int>(sizeof(a) / sizeof((a)[0]))

template <typename T>
uint8_t clamp_css_byte(T i) {  // Clamp to integer 0 .. 255.
  i = round(i);
  return i < 0 ? 0 : i > 255 ? 255 : i;
}

template <typename T>
float clamp_css_float(T f) {  // Clamp to float 0.0 .. 1.0.
  return f < 0 ? 0 : f > 1 ? 1 : f;
}
bool parse_css_int(const std::string& str,
                   uint8_t& output) {  // int or percentage.
  int64_t i = 0;
  if (str.length() && str[str.length() - 1] == '%') {
    if (UNLIKELY(!base::StringToInt(str.substr(0, str.length() - 1), i, 10))) {
      return false;
    }
    output = clamp_css_byte(i / 100.0f * 255.0f);
    return true;
  } else {
    if (UNLIKELY(!base::StringToInt(str.substr(0, str.length()), i, 10))) {
      return false;
    }
    output = clamp_css_byte(i);
    return true;
  }
}

bool parse_css_float(const std::string& str,
                     float& output) {  // float or percentage.
  double d = 0;
  if (str.length() && str[str.length() - 1] == '%') {
    if (UNLIKELY(
            !base::StringToDouble(str.substr(0, str.length() - 1), d, true))) {
      return false;
    }
    output = clamp_css_float(d / 100.0f);
    return true;
  } else {
    if (UNLIKELY(!base::StringToDouble(str.substr(0, str.length()), d, true))) {
      return false;
    }
    output = clamp_css_float(d);
    return true;
  }
}

float css_hue_to_rgb(float m1, float m2, float h) {
  if (h < 0.0f) {
    h += 1.0f;
  } else if (h > 1.0f) {
    h -= 1.0f;
  }

  if (h * 6.0f < 1.0f) {
    return m1 + (m2 - m1) * h * 6.0f;
  }
  if (h * 2.0f < 1.0f) {
    return m2;
  }
  if (h * 3.0f < 2.0f) {
    return m1 + (m2 - m1) * (2.0 / 3.0 - h) * 6.0f;
  }
  return m1;
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

bool CSSColor::Parse(const std::string& color_str, CSSColor& color) {
  if (color_str.empty()) return false;
  std::string str = color_str;

  // Remove all whitespace, not compliant, but should just be more accepting.
  str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

  // Convert to lowercase.
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);

  // #abc and #abc123 syntax.
  if (str.length() && str[0] == '#') {
    if (str.length() == 4) {
      int64_t iv = 0;
      if (UNLIKELY(!base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xfff)) {
        return false;
      } else {
        color = {
            static_cast<uint8_t>(((iv & 0xf00) >> 4) | ((iv & 0xf00) >> 8)),
            static_cast<uint8_t>((iv & 0xf0) | ((iv & 0xf0) >> 4)),
            static_cast<uint8_t>((iv & 0xf) | ((iv & 0xf) << 4)), 1};
        return true;
      }
    } else if (str.length() == 5) {
      // format like #abcd will be parserd as #aabbccdd
      int64_t iv = 0;
      if (UNLIKELY(!base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xffff)) {
        return false;
      } else {
        color = {
            static_cast<uint8_t>(((iv & 0xf000) >> 8) | ((iv & 0xf000) >> 12)),
            static_cast<uint8_t>(((iv & 0xf00) >> 4) | ((iv & 0xf00) >> 8)),
            static_cast<uint8_t>((iv & 0xf0) | ((iv & 0xf0) >> 4)),
            ((iv & 0xf) | ((iv & 0xf) << 4)) / 255.0f};
        return true;
      }
    } else if (str.length() == 7) {
      int64_t iv = 0;
      if (UNLIKELY(!base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xffffff)) {
        return false;  // Covers NaN.
      } else {
        color = {static_cast<uint8_t>((iv & 0xff0000) >> 16),
                 static_cast<uint8_t>((iv & 0xff00) >> 8),
                 static_cast<uint8_t>(iv & 0xff), 1};
        return true;
      }
    } else if (str.length() == 9) {
      int64_t iv = 0;
      if (UNLIKELY(!base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xffffffff)) {
        return false;  // Covers NaN.
      } else {
        color = {static_cast<uint8_t>((iv & 0xff000000) >> 24),
                 static_cast<uint8_t>((iv & 0xff0000) >> 16),
                 static_cast<uint8_t>((iv & 0xff00) >> 8),
                 static_cast<uint8_t>(iv & 0xff) / 255.0f};
        return true;
      }
    }

    return false;
  }

  size_t op = str.find_first_of('('), ep = str.find_first_of(')');
  if (op != std::string::npos && ep + 1 == str.length()) {
    const std::string fname = str.substr(0, op);
    const std::vector<std::string> params =
        split(str.substr(op + 1, ep - (op + 1)), ',');

    float alpha = 1.0f;

    if (fname == "rgba" || fname == "rgb") {
      if (fname == "rgba") {
        if (params.size() != 4) {
          return false;
        }
        if (UNLIKELY(!parse_css_float(params.back(), alpha))) {
          return false;
        }
      } else {
        if (params.size() != 3) {
          return false;
        }
      }
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
      if (UNLIKELY(!parse_css_int(params[0], r) ||
                   !parse_css_int(params[1], g) ||
                   !parse_css_int(params[2], b))) {
        return false;
      }
      color = {r, g, b, alpha};
      return true;

    } else if (fname == "hsla" || fname == "hsl") {
      if (fname == "hsla") {
        if (params.size() != 4) {
          return false;
        }
        if (UNLIKELY(!parse_css_float(params.back(), alpha))) {
          return false;
        }
      } else {
        if (params.size() != 3) {
          return false;
        }
      }
      double h = 0;
      if (UNLIKELY(!base::StringToDouble(params[0], h, true))) {
        return false;
      }

      // NOTE(deanm): According to the CSS spec s/l should only be
      // percentages, but we don't bother and let float or percentage.
      float s = 0.f;
      float l = 0.f;
      if (UNLIKELY(!parse_css_float(params[1], s) ||
                   !parse_css_float(params[2], l))) {
        return false;
      }

      color = CreateFromHSLA(h, s * 100, l * 100, alpha);
      return true;
    }
  }
  // Check in named colors
  return ParseNamedColor(str, color);
}

bool CSSColor::ParseNamedColor(const std::string& color_str, CSSColor& color) {
  auto iter = GetTokenValue(color_str.c_str(),
                            static_cast<unsigned>(color_str.length()));
  if (iter != nullptr && IsColorIdentifier(iter->type)) {
    color = CreateFromKeyword(iter->type);
    return true;
  }
  return false;
}

CSSColor CSSColor::CreateFromHSLA(float h, float s, float l, float a) {
  h /= 360.0f;
  s /= 100.0f;
  l /= 100.0f;
  while (h < 0.0f) h++;
  while (h > 1.0f) h--;

  float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
  float m1 = l * 2.0f - m2;

  return CSSColor{
      clamp_css_byte(css_hue_to_rgb(m1, m2, h + 1.0f / 3.0f) * 255.0f),
      clamp_css_byte(css_hue_to_rgb(m1, m2, h) * 255.0f),
      clamp_css_byte(css_hue_to_rgb(m1, m2, h - 1.0f / 3.0f) * 255.0f),
      clamp_css_float(a)};
}

CSSColor CSSColor::CreateFromRGBA(float r, float g, float b, float a) {
  return CSSColor{clamp_css_byte(r), clamp_css_byte(g), clamp_css_byte(b),
                  clamp_css_float(a)};
}

CSSColor CSSColor::CreateFromKeyword(TokenType type) {
  int index = static_cast<int>(type) - static_cast<int>(TokenType::TRANSPARENT);
  static const CSSColor colors[] =
      // [TokenType::TRANSPARENT, ..., TokenType::YELLOWGREEN]
      {{0, 0, 0, 0},       {240, 248, 255, 1}, {250, 235, 215, 1},
       {0, 255, 255, 1},   {127, 255, 212, 1}, {240, 255, 255, 1},
       {245, 245, 220, 1}, {255, 228, 196, 1}, {0, 0, 0, 1},
       {255, 235, 205, 1}, {0, 0, 255, 1},     {138, 43, 226, 1},
       {165, 42, 42, 1},   {222, 184, 135, 1}, {95, 158, 160, 1},
       {127, 255, 0, 1},   {210, 105, 30, 1},  {255, 127, 80, 1},
       {100, 149, 237, 1}, {255, 248, 220, 1}, {220, 20, 60, 1},
       {0, 255, 255, 1},   {0, 0, 139, 1},     {0, 139, 139, 1},
       {184, 134, 11, 1},  {169, 169, 169, 1}, {0, 100, 0, 1},
       {169, 169, 169, 1}, {189, 183, 107, 1}, {139, 0, 139, 1},
       {85, 107, 47, 1},   {255, 140, 0, 1},   {153, 50, 204, 1},
       {139, 0, 0, 1},     {233, 150, 122, 1}, {143, 188, 143, 1},
       {72, 61, 139, 1},   {47, 79, 79, 1},    {47, 79, 79, 1},
       {0, 206, 209, 1},   {148, 0, 211, 1},   {255, 20, 147, 1},
       {0, 191, 255, 1},   {105, 105, 105, 1}, {105, 105, 105, 1},
       {30, 144, 255, 1},  {178, 34, 34, 1},   {255, 250, 240, 1},
       {34, 139, 34, 1},   {255, 0, 255, 1},   {220, 220, 220, 1},
       {248, 248, 255, 1}, {255, 215, 0, 1},   {218, 165, 32, 1},
       {128, 128, 128, 1}, {0, 128, 0, 1},     {173, 255, 47, 1},
       {128, 128, 128, 1}, {240, 255, 240, 1}, {255, 105, 180, 1},
       {205, 92, 92, 1},   {75, 0, 130, 1},    {255, 255, 240, 1},
       {240, 230, 140, 1}, {230, 230, 250, 1}, {255, 240, 245, 1},
       {124, 252, 0, 1},   {255, 250, 205, 1}, {173, 216, 230, 1},
       {240, 128, 128, 1}, {224, 255, 255, 1}, {250, 250, 210, 1},
       {211, 211, 211, 1}, {144, 238, 144, 1}, {211, 211, 211, 1},
       {255, 182, 193, 1}, {255, 160, 122, 1}, {32, 178, 170, 1},
       {135, 206, 250, 1}, {119, 136, 153, 1}, {119, 136, 153, 1},
       {176, 196, 222, 1}, {255, 255, 224, 1}, {0, 255, 0, 1},
       {50, 205, 50, 1},   {250, 240, 230, 1}, {255, 0, 255, 1},
       {128, 0, 0, 1},     {102, 205, 170, 1}, {0, 0, 205, 1},
       {186, 85, 211, 1},  {147, 112, 219, 1}, {60, 179, 113, 1},
       {123, 104, 238, 1}, {0, 250, 154, 1},   {72, 209, 204, 1},
       {199, 21, 133, 1},  {25, 25, 112, 1},   {245, 255, 250, 1},
       {255, 228, 225, 1}, {255, 228, 181, 1}, {255, 222, 173, 1},
       {0, 0, 128, 1},     {253, 245, 230, 1}, {128, 128, 0, 1},
       {107, 142, 35, 1},  {255, 165, 0, 1},   {255, 69, 0, 1},
       {218, 112, 214, 1}, {238, 232, 170, 1}, {152, 251, 152, 1},
       {175, 238, 238, 1}, {219, 112, 147, 1}, {255, 239, 213, 1},
       {255, 218, 185, 1}, {205, 133, 63, 1},  {255, 192, 203, 1},
       {221, 160, 221, 1}, {176, 224, 230, 1}, {128, 0, 128, 1},
       {255, 0, 0, 1},     {188, 143, 143, 1}, {65, 105, 225, 1},
       {139, 69, 19, 1},   {250, 128, 114, 1}, {244, 164, 96, 1},
       {46, 139, 87, 1},   {255, 245, 238, 1}, {160, 82, 45, 1},
       {192, 192, 192, 1}, {135, 206, 235, 1}, {106, 90, 205, 1},
       {112, 128, 144, 1}, {112, 128, 144, 1}, {255, 250, 250, 1},
       {0, 255, 127, 1},   {70, 130, 180, 1},  {210, 180, 140, 1},
       {0, 128, 128, 1},   {216, 191, 216, 1}, {255, 99, 71, 1},
       {64, 224, 208, 1},  {238, 130, 238, 1}, {245, 222, 179, 1},
       {255, 255, 255, 1}, {245, 245, 245, 1}, {255, 255, 0, 1},
       {154, 205, 50, 1}};
  DCHECK(index >= 0 && index < ARRAY_SIZE(colors));
  return colors[index];
}

unsigned int CSSColor::Cast() const {
  unsigned int color =
      (0xffffffff & b_) | ((0xffffffff & g_) << 8) | ((0xffffffff & r_) << 16) |
      ((0xffffffff & (static_cast<unsigned char>(a_ * 255))) << 24);
  return color;
}

bool CSSColor::IsColorIdentifier(TokenType type) {
  return type >= TokenType::TRANSPARENT && type <= TokenType::YELLOWGREEN;
}

}  // namespace tasm
}  // namespace lynx

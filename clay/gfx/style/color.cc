// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/color.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "clay/fml/logging.h"
#include "clay/gfx/style/color_gen.inl"

namespace clay {

namespace {

static const uint32_t lightened_black = 0xFF545454;
static const uint32_t darkened_white = 0xFFABABAB;

template <class T>
T clamp(T value, T min_val, T max_val) {
  FML_DCHECK(min_val <= max_val);
  if (value < min_val) {
    return min_val;
  }
  return value > max_val ? max_val : value;
}

uint32_t lerp(float a, float b, float t) {
  // t with negative values and values greater than 1.0 are invalid.
  // FML_DCHECK(t >= 0.0 && t <= 1.0);
  // a * (1-t) + b * t
  return a + (b - a) * t;
}

// ------- Copy from Lynx Start ------- //
template <typename T>
uint8_t ClampCssByte(T i) {  // Clamp to integer 0 .. 255.
  i = round(i);
  return i < 0 ? 0 : i > 255 ? 255 : i;
}

template <typename T>
float ClampCssFloat(T f) {  // Clamp to float 0.0 .. 1.0.
  return f < 0 ? 0 : f > 1 ? 1 : f;
}
bool ParseCssInt(const std::string& str,
                 uint8_t* output) {  // int or percentage.
  int64_t i = 0;
  if (str.length() && str[str.length() - 1] == '%') {
    if (UNLIKELY(
            !lynx::base::StringToInt(str.substr(0, str.length() - 1), i, 10))) {
      return false;
    }
    *output = ClampCssByte(i / 100.0f * 255.0f);
    return true;
  } else {
    if (UNLIKELY(
            !lynx::base::StringToInt(str.substr(0, str.length()), i, 10))) {
      return false;
    }
    *output = ClampCssByte(i);
    return true;
  }
}

bool ParseCssFloat(const std::string& str,
                   float* output) {  // float or percentage.
  float d = 0;
  if (str.length() && str[str.length() - 1] == '%') {
    if (UNLIKELY(
            !lynx::base::StringToFloat(str.substr(0, str.length() - 1), d))) {
      return false;
    }
    *output = ClampCssFloat(d / 100.0f);
    return true;
  } else {
    if (UNLIKELY(!lynx::base::StringToFloat(str.substr(0, str.length()), d))) {
      return false;
    }
    *output = ClampCssFloat(d);
    return true;
  }
}

float CssHueToRgb(float m1, float m2, float h) {
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

// ------- Copy from Lynx End ------- //

}  // namespace

// static
Color Color::Lerp(Color a, Color b, float t) {
  return ARGBColor(clamp(lerp(a.Alpha(), b.Alpha(), t), 0u, 255u),
                   clamp(lerp(a.Red(), b.Red(), t), 0u, 255u),
                   clamp(lerp(a.Green(), b.Green(), t), 0u, 255u),
                   clamp(lerp(a.Blue(), b.Blue(), t), 0u, 255u));
}

// static
Color Color::AlphaBlend(Color foreground, Color background) {
  int alpha = foreground.Alpha();
  if (alpha == 0x00) {
    return background;
  }
  int invAlpha = 0xFF - alpha;
  int backAlpha = background.Alpha();
  if (backAlpha == 0xFF) {
    return ARGBColor(
        0xFF, (alpha * foreground.Red() + invAlpha * background.Red()) / 0xFF,
        (alpha * foreground.Green() + invAlpha * background.Green()) / 0xFF,
        (alpha * foreground.Blue() + invAlpha * background.Blue()) / 0xFF);
  } else {
    backAlpha = (backAlpha * invAlpha) / 0xFF;
    int outAlpha = alpha + backAlpha;
    FML_DCHECK(outAlpha != 0x00);
    return ARGBColor(
        outAlpha,
        (foreground.Red() * alpha + background.Red() * backAlpha) / outAlpha,
        (foreground.Green() * alpha + background.Green() * backAlpha) /
            outAlpha,
        (foreground.Blue() * alpha + background.Blue() * backAlpha) / outAlpha);
  }
}

// static
bool Color::Parse(const std::string& color_str, Color* color) {
  if (color_str.empty()) {
    return false;
  }

  // Remove all whitespace, not compliant, but should just be more accepting.
  std::string str = lynx::base::RemoveSpaces(color_str);

  // Convert to lowercase.
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);

  // #abc and #abc123 syntax.
  if (str.length() && str[0] == '#') {
    if (str.length() == 4) {
      int64_t iv = 0;
      if (UNLIKELY(!lynx::base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xfff)) {
        return false;
      } else {
        *color = RGBOColor(((iv & 0xf00) >> 4) | ((iv & 0xf00) >> 8),
                           (iv & 0xf0) | ((iv & 0xf0) >> 4),
                           (iv & 0xf) | ((iv & 0xf) << 4), 1);
        return true;
      }
    } else if (str.length() == 7) {
      int64_t iv = 0;
      if (UNLIKELY(!lynx::base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xffffff)) {
        return false;  // Covers NaN.
      } else {
        *color =
            RGBOColor((iv & 0xff0000) >> 16, (iv & 0xff00) >> 8, iv & 0xff, 1);
        return true;
      }
    } else if (str.length() == 9) {
      int64_t iv = 0;
      if (UNLIKELY(!lynx::base::StringToInt(str.substr(1), iv, 16))) {
        return false;
      }
      if (!(iv >= 0 && iv <= 0xffffffff)) {
        return false;  // Covers NaN.
      } else {
        *color = RGBOColor((iv & 0xff000000) >> 24, (iv & 0xff0000) >> 16,
                           (iv & 0xff00) >> 8, (iv & 0xff) / 255.0);
        return true;
      }
    }

    return false;
  }

  size_t op = str.find_first_of('('), ep = str.find_first_of(')');
  if (op != std::string::npos && ep + 1 == str.length()) {
    const std::string fname = str.substr(0, op);
    const std::vector<std::string> params =
        lynx::base::SplitString<std::string, std::string>(
            str.substr(op + 1, ep - (op + 1)), ",");

    float alpha = 1.0f;

    if (fname == "rgba" || fname == "rgb") {
      if (fname == "rgba") {
        if (params.size() != 4) {
          return false;
        }
        if (UNLIKELY(!ParseCssFloat(params.back(), &alpha))) {
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
      if (UNLIKELY(!ParseCssInt(params[0], &r) || !ParseCssInt(params[1], &g) ||
                   !ParseCssInt(params[2], &b))) {
        return false;
      }
      *color = RGBOColor(r, g, b, alpha);
      return true;

    } else if (fname == "hsla" || fname == "hsl") {
      if (fname == "hsla") {
        if (params.size() != 4) {
          return false;
        }
        if (UNLIKELY(!ParseCssFloat(params[3], &alpha))) {
          return false;
        }
      } else {
        if (params.size() != 3) {
          return false;
        }
      }
      float h = 0.f;
      if (UNLIKELY(!lynx::base::StringToFloat(params[0], h))) {
        return false;
      }
      h /= 360.0f;
      while (h < 0.0f) {
        h++;
      }
      while (h > 1.0f) {
        h--;
      }

      // NOTE(deanm): According to the CSS spec s/l should only be
      // percentages, but we don't bother and let float or percentage.
      float s = 0.f;
      float l = 0.f;
      if (UNLIKELY(!ParseCssFloat(params[1], &s) ||
                   !ParseCssFloat(params[2], &l))) {
        return false;
      }

      float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
      float m1 = l * 2.0f - m2;

      *color = RGBOColor(
          ClampCssByte(CssHueToRgb(m1, m2, h + 1.0f / 3.0f) * 255.0f),
          ClampCssByte(CssHueToRgb(m1, m2, h) * 255.0f),
          ClampCssByte(CssHueToRgb(m1, m2, h - 1.0f / 3.0f) * 255.0f), alpha);
      return true;
    }
  }

  auto tk = ColorHash::find(str.c_str(), str.size());
  if (tk) {
    *color = tk->color;
    return true;
  }
  return false;
}

void Color::GetARGB(float* a, float* r, float* g, float* b) const {
  *a = Alpha() / 255.0f;
  *r = Red() / 255.0f;
  *g = Green() / 255.0f;
  *b = Blue() / 255.0f;
}

Color Color::Light() const {
  // Hardcode this common case for speed.
  if (argb == 0xFF000000) {
    return Color(lightened_black);
  }

  const float scaleFactor = nextafterf(256.0f, 0.0f);

  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 0.f;
  GetARGB(&a, &r, &g, &b);

  float v = std::max(r, std::max(g, b));

  if (v == 0.0f) {
    // Lightened black with alpha.
    return Color::ARGBColor(Alpha(), 0x54, 0x54, 0x54);
  }

  float multiplier = std::min(1.0f, v + 0.33f) / v;

  return Color::ARGBColor(Alpha(),
                          static_cast<int>(multiplier * r * scaleFactor),
                          static_cast<int>(multiplier * g * scaleFactor),
                          static_cast<int>(multiplier * b * scaleFactor));
}

Color Color::Dark() const {
  // Hardcode this common case for speed.
  if (argb == 0xFFFFFFFF) {
    return Color(darkened_white);
  }

  const float scaleFactor = nextafterf(256.0f, 0.0f);

  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 0.f;
  GetARGB(&a, &r, &g, &b);

  float v = std::max(r, std::max(g, b));
  float multiplier = std::max(0.0f, (v - 0.33f) / v);

  return Color::ARGBColor(Alpha(),
                          static_cast<int>(multiplier * r * scaleFactor),
                          static_cast<int>(multiplier * g * scaleFactor),
                          static_cast<int>(multiplier * b * scaleFactor));
}

#ifndef NDEBUG
std::string Color::ToString() const {
  std::stringstream ss;
  ss << "argb(" << Alpha() << "," << Red() << "," << Green() << "," << Blue()
     << ")";
  return ss.str();
}
#endif

}  // namespace clay

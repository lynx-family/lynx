// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_utils.h"

#include <cmath>

#include "base/include/vector.h"

namespace lynx {
namespace tasm {
namespace {

using vec2 = std::pair<float, float>;

// Compute the radius to the closest/farthest side (depending on the compare
// functor).
vec2 RadiusToSide(float px, float py, float sx, float sy,
                  RadialGradientShapeType shape,
                  bool (*compare)(float, float)) {
  float dx1 = fabs(px);
  float dy1 = fabs(py);
  float dx2 = fabs(px - sx);
  float dy2 = fabs(py - sy);

  float dx = compare(dx1, dx2) ? dx1 : dx2;
  float dy = compare(dy1, dy2) ? dy1 : dy2;

  if (shape == RadialGradientShapeType::kCircle)
    return compare(dx, dy) ? vec2(dx, dx) : vec2(dy, dy);

  return vec2(dx, dy);
}

// Compute the radius of an ellipse
inline vec2 EllipseRadius(float offset_x, float offset_y, float aspect_ratio) {
  // If the aspect_ratio is 0 or infinite, the ellipse is completely flat.
  if (aspect_ratio == 0 || std::isinf(aspect_ratio) ||
      std::isnan(aspect_ratio)) {
    return vec2(0, 0);
  }

  float a = sqrtf(offset_x * offset_x +
                  offset_y * offset_y * aspect_ratio * aspect_ratio);
  return vec2(a, a / aspect_ratio);
}

// Compute the radius to the closest/farthest corner (depending on the compare
// functor).
vec2 RadiusToCorner(float px, float py, float sx, float sy,
                    RadialGradientShapeType shape,
                    bool (*compare)(float, float)) {
  vec2 corners[] = {{0.f, 0.f}, {sx, 0}, {sx, sy}, {0, sy}};

  unsigned corner_index = 0;
  float distance = hypotf(px - corners[corner_index].first,
                          py - corners[corner_index].second);
  for (unsigned i = 1; i < std::size(corners); ++i) {
    float new_distance = hypotf(px - corners[i].first, py - corners[i].second);
    if (compare(new_distance, distance)) {
      corner_index = i;
      distance = new_distance;
    }
  }

  if (shape == RadialGradientShapeType::kCircle) {
    return vec2(distance, distance);
  }

  // If the end shape is an ellipse, the gradient-shape has the same ratio of
  // width to height that it would if closest-side or farthest-side were
  // specified, as appropriate.
  const vec2 side_radius =
      RadiusToSide(px, py, sx, sy, RadialGradientShapeType::kEllipse, compare);

  return EllipseRadius(corners[corner_index].first - px,
                       corners[corner_index].second - py,
                       side_radius.first / side_radius.second);
}
}  // namespace

vec2 GetRadialGradientRadius(RadialGradientShapeType shape,
                             RadialGradientSizeType shape_size, float cx,
                             float cy, float sx, float sy) {
  vec2 radius = {0, 0};
  switch (shape_size) {
    case RadialGradientSizeType::kClosestSide:
      radius = RadiusToSide(cx, cy, sx, sy, shape,
                            [](float a, float b) { return a < b; });
      break;
    case RadialGradientSizeType::kClosestCorner:
      radius = RadiusToCorner(cx, cy, sx, sy, shape,
                              [](float a, float b) { return a < b; });
      break;
    case RadialGradientSizeType::kFarthestSide:
      radius = RadiusToSide(cx, cy, sx, sy, shape,
                            [](float a, float b) { return a > b; });
      break;
    case RadialGradientSizeType::kFarthestCorner:
      radius = RadiusToCorner(cx, cy, sx, sy, shape,
                              [](float a, float b) { return a > b; });
      break;
    default:
      break;
  }
  return radius;
}

bool ParseStyleDeclarationList(const char* content, uint32_t content_length,
                               DeclarationListConsumeFunction consume_func) {
  uint32_t current{0};
  const char kLeftBracket = '(';
  const char kRightBracket = ')';
  const char kSemiColon = ';';
  const char kColon = ':';
  const char kWhiteSpace = ' ';

  while (current < content_length) {
    // key
    while (current < content_length) {
      char c = content[current];
      if (c != kSemiColon && c != kWhiteSpace) {
        break;
      }
      current++;
    }

    auto key_start = current;
    auto key_end = key_start;

    //':', advance to colon or whitespace or semicolon(bad case)
    while (current < content_length) {
      char c = content[current];
      if ((c == kColon || c == kWhiteSpace || c == kSemiColon)) {
        if (key_end == key_start) {
          key_end = current;
        }
        if (c != kWhiteSpace) {
          break;
        }
      }
      current++;
    }

    //
    if (content[current] != kColon) {
      // a bad case like: "background;red;width:1px"
      continue;
    }

    // Advance ':'
    current++;

    // value
    // skip whitespace after ':', for example: "background: red"
    auto value_start = 0;

    while (current < content_length) {
      char c = content[current];
      if (value_start == 0 && c != kWhiteSpace) {
        value_start = current;
      }

      // handle '('
      if (c == kLeftBracket) {
        base::InlineStack<char, 16> brackets;
        brackets.push(kLeftBracket);
        current++;
        while (current < content_length && !brackets.empty()) {
          if (content[current] == kLeftBracket) {
            brackets.push(kLeftBracket);
          }
          if (content[current] == kRightBracket) {
            brackets.pop();
          }
          if (brackets.empty()) {
            break;
          }
          current++;
        }
        if (!brackets.empty()) {
          return false;
        }

        continue;
      }

      // handle '\'', '\"'
      if (c == '\'' || c == '\"') {
        char boundary = c;
        current++;
        while (current < content_length && content[current] != boundary) {
          current++;
        }

        if (current >= content_length) {
          // Unterminated string without `'` at end , like 'xxxxx
          return false;
        }
        continue;
      }

      // end of semicolon
      if (c == kSemiColon) {
        break;
      }
      current++;
    }

    auto value_end = current;

    if (key_end < content_length && value_end <= content_length) {
      consume_func(content + key_start, key_end - key_start,
                   content + value_start, value_end - value_start);
    }
  }
  return true;
}

ClassList SplitClasses(const char* content, size_t length) {
  size_t i = 0, len = length, start = i;
  ClassList output;
  while (i < len) {
    char current_char = content[i];
    if (current_char == ' ' && start == i) {
      i++;
      start = i;
      // skip whitespace
      continue;
    }

    size_t end = 0;
    if (current_char == ' ' || current_char == '\0') {
      end = i;
    } else if (i == len - 1) {
      // end of the string
      end = len;
    }

    if (end > start) {
      output.emplace_back(content + start, end - start);
      start = i + 1;
    }
    i++;
  }
  return output;
}

}  // namespace tasm
}  // namespace lynx

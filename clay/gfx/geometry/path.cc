/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "clay/gfx/geometry/path.h"

#include "clay/fml/logging.h"

namespace clay {

static inline bool is_between(int c, int min, int max) {
  return (unsigned)(c - min) <= (unsigned)(max - min);
}

static inline bool is_ws(int c) { return is_between(c, 1, 32); }

static inline bool is_digit(int c) { return is_between(c, '0', '9'); }

static inline bool is_sep(int c) { return is_ws(c) || c == ','; }

static inline bool is_lower(int c) { return is_between(c, 'a', 'z'); }

static inline int to_upper(int c) { return c - 'a' + 'A'; }

static const char* skip_ws(const char str[]) {
  FML_DCHECK(str);
  while (is_ws(*str)) str++;
  return str;
}

static const char* skip_sep(const char str[]) {
  if (!str) {
    return nullptr;
  }
  while (is_sep(*str)) str++;
  return str;
}

const char* FindScalar(const char str[], float* value) {
  FML_DCHECK(str);
  str = skip_ws(str);

  char* stop;
  float v = (float)strtod(str, &stop);
  if (str == stop) {
    return nullptr;
  }
  if (value) {
    *value = v;
  }
  return stop;
}

const char* FindScalars(const char str[], float value[], int count) {
  FML_DCHECK(count >= 0);

  if (count > 0) {
    for (;;) {
      str = FindScalar(str, value);
      if (--count == 0 || str == nullptr) break;

      // keep going
      str = skip_sep(str);
      if (value) value += 1;
    }
  }
  return str;
}

static const char* find_points(const char str[], GrPoint value[], int count,
                               bool isRelative, GrPoint* relative) {
#ifndef ENABLE_SKITY
  str = FindScalars(str, &value[0].fX, count * 2);
  if (isRelative) {
    for (int index = 0; index < count; index++) {
      value[index].fX += relative->fX;
      value[index].fY += relative->fY;
    }
  }
#else
  for (int i = 0; i < count; i++) {
    str = FindScalars(str, &value[i].x, 2);
    if (isRelative) {
      for (int index = 0; index < count; index++) {
        value[index].x += relative->x;
        value[index].y += relative->y;
      }
    }
  }
#endif  // ENABLE_SKITY
  return str;
}

static const char* find_scalar(const char str[], float* value, bool isRelative,
                               float relative) {
  str = FindScalar(str, value);
  if (!str) {
    return nullptr;
  }
  if (isRelative) {
    *value += relative;
  }
  str = skip_sep(str);
  return str;
}

// https://www.w3.org/TR/SVG11/paths.html#PathDataBNF
//
// flag:
//    "0" | "1"
static const char* find_flag(const char str[], bool* value) {
  if (!str) {
    return nullptr;
  }
  if (str[0] != '1' && str[0] != '0') {
    return nullptr;
  }
  *value = str[0] != '0';
  str = skip_sep(str + 1);
  return str;
}

bool PathBuilder::ParsePathString(const char data[], GrPath* result) {
  GrPath path;
#ifndef ENABLE_SKITY
  GrPoint first = {0, 0};
  GrPoint c = {0, 0};
  GrPoint last_c = {0, 0};
#else
  GrPoint first = {0, 0, 0, 0};
  GrPoint c = {0, 0, 0, 0};
  GrPoint last_c = {0, 0, 0, 0};
#endif  // ENABLE_SKITY
  GrPoint points[3];
  char op = '\0';
  char previousOp = '\0';
  bool relative = false;
  for (;;) {
    if (!data) {
      // Truncated data
      return false;
    }
    data = skip_ws(data);
    if (data[0] == '\0') {
      break;
    }
    char ch = data[0];
    if (is_digit(ch) || ch == '-' || ch == '+' || ch == '.') {
      if (op == '\0' || op == 'Z') {
        return false;
      }
    } else if (is_sep(ch)) {
      data = skip_sep(data);
    } else {
      op = ch;
      relative = false;
      if (is_lower(op)) {
        op = (char)to_upper(op);
        relative = true;
      }
      data++;
      data = skip_sep(data);
    }
    switch (op) {
      case 'M':
        data = find_points(data, points, 1, relative, &c);
        PATH_MOVE_TO_POINT(path, points[0]);
        previousOp = '\0';
        op = 'L';
        c = points[0];
        break;
      case 'L':
        data = find_points(data, points, 1, relative, &c);
        PATH_LINE_TO_POINT(path, points[0]);
        c = points[0];
        break;
      case 'H': {
        float x;
        data = find_scalar(data, &x, relative, POINT_GET_X(c));
        PATH_LINE_TO(path, x, POINT_GET_Y(c));
        POINT_SET_X(c, x);
      } break;
      case 'V': {
        float y;
        data = find_scalar(data, &y, relative, POINT_GET_Y(c));
        PATH_LINE_TO(path, POINT_GET_X(c), y);
        POINT_SET_Y(c, y);
      } break;
      case 'C':
        data = find_points(data, points, 3, relative, &c);
        goto cubicCommon;
      case 'S':
        data = find_points(data, &points[1], 2, relative, &c);
        points[0] = c;
        if (previousOp == 'C' || previousOp == 'S') {
          auto c_x = POINT_GET_X(c);
          auto c_y = POINT_GET_Y(c);
          auto last_c_x = POINT_GET_X(last_c);
          auto last_c_y = POINT_GET_Y(last_c);
          auto new_x = POINT_GET_X(points[0]) - (last_c_x - c_x);
          auto new_y = POINT_GET_Y(points[0]) - (last_c_y - c_y);
          POINT_SET_X(points[0], new_x);
          POINT_SET_Y(points[0], new_y);
        }
      cubicCommon:
        PATH_CUBIC_TO_POINT(path, points[0], points[1], points[2]);
        last_c = points[1];
        c = points[2];
        break;
      case 'Q':  // Quadratic Bezier Curve
        data = find_points(data, points, 2, relative, &c);
        goto quadraticCommon;
      case 'T':
        data = find_points(data, &points[1], 1, relative, &c);
        points[0] = c;
        if (previousOp == 'Q' || previousOp == 'T') {
          auto c_x = POINT_GET_X(c);
          auto c_y = POINT_GET_Y(c);
          auto last_c_x = POINT_GET_X(last_c);
          auto last_c_y = POINT_GET_Y(last_c);
          auto new_x = POINT_GET_X(points[0]) - (last_c_x - c_x);
          auto new_y = POINT_GET_Y(points[0]) - (last_c_y - c_y);
          POINT_SET_X(points[0], new_x);
          POINT_SET_Y(points[0], new_y);
        }
      quadraticCommon:
        PATH_QUAD_TO_POINT(path, points[0], points[1]);
        last_c = points[0];
        c = points[1];
        break;
      case 'A': {
        GrPoint radii;
        float angle;
        bool largeArc, sweep;
        if ((data = find_points(data, &radii, 1, false, nullptr)) &&
            (data = skip_sep(data)) &&
            (data = find_scalar(data, &angle, false, 0)) &&
            (data = skip_sep(data)) && (data = find_flag(data, &largeArc)) &&
            (data = skip_sep(data)) && (data = find_flag(data, &sweep)) &&
            (data = skip_sep(data)) &&
            (data = find_points(data, &points[0], 1, relative, &c))) {
          PATH_ARC_TO(path, radii, angle, (GrPath::ArcSize)largeArc,
                      (GrPathDirection)!sweep, points[0]);
          PATH_GET_LAST_POINT(path, &c);
        }
      } break;
      case 'Z':
        PATH_CLOSE(path);
        c = first;
        break;
      case '~': {
        GrPoint args[2];
        data = find_points(data, args, 2, false, nullptr);
        PATH_MOVE_TO_POINT(path, args[0]);
        PATH_LINE_TO_POINT(path, args[1]);
      } break;
      default:
        return false;
    }
    if (previousOp == 0) {
      first = c;
    }
    previousOp = op;
  }
  // we're good, go ahead and swap in the result
  PATH_SWAP(result, path);
  return true;
}

}  // namespace clay

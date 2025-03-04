// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/path_parser.h"

#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace lepus {

PathVector ParseValuePath(const std::string &path) {
  PathVector path_array;
  std::stringstream result;
  bool array_start_ = false;
  bool num_in_array_ = false;
  int digits = 0;
  std::size_t length = path.length();
  for (std::size_t index = 0; index < length; ++index) {
    char c = path[index];
    if (c == '.') {
      std::string ss = result.str();
      result.clear();
      result.str("");
      if (ss.length() > 0) {
        path_array.emplace_back(std::move(ss));
      }
    } else if (c == '[') {
      if (array_start_) {
        LOGE("Data Path Error, Path can not have nested []. Path: " << path);
        path_array.clear();
        return path_array;  // Guarantee NRVO
      }
      std::string ss = result.str();
      result.clear();
      result.str("");
      if (ss.length() > 0) {
        path_array.emplace_back(std::move(ss));
      }
      if (path_array.empty()) {
        LOGE("Data Path Error, Path can not start with []. Path: " << path);
        path_array.clear();
        return path_array;  // Guarantee NRVO
      }
      array_start_ = true;
      num_in_array_ = false;
    } else if (c == ']') {
      if (!num_in_array_) {
        LOGE("Data Path Error, Must has number in []. Path: " << path);
        path_array.clear();
        return path_array;  // Guarantee NRVO
      }
      array_start_ = false;
      path_array.emplace_back(std::to_string(digits));
      digits = 0;

      // there may have escape number in brackets like:
      // a[\1]
      // should clear result here
      result.clear();
      result.str("");
    } else if (c == '\\') {
      if (index == length - 1) {
        result << c;
        break;
      }

      char next = path[index + 1];
      if (next == '[' || next == ']' || next == '.') {
        // escape special char
        result << next;
        index++;
      } else {
        result << '\\';
      }
    } else if (array_start_) {
      if (c < '0' || c > '9') {
        LOGE("Data Path Error, Only number 0-9 could be inside []. Path: "
             << path);
        path_array.clear();
        return path_array;  // Guarantee NRVO
      }
      num_in_array_ = true;
      digits = 10 * digits + (c - '0');
    } else {
      result << c;
    }
  }
  if (array_start_) {
    LOGE("Data Path Error, [] should appear in pairs. Path: " << path);
    path_array.clear();
    return path_array;  // Guarantee NRVO
  }
  std::string ss = result.str();
  if (ss.length() > 0) {
    path_array.emplace_back(std::move(ss));
  }
  return path_array;
}

}  // namespace lepus
}  // namespace lynx

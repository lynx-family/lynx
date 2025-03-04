// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PATH_UTILS_H_
#define BASE_INCLUDE_PATH_UTILS_H_

#include <sstream>
#include <string>
#include <vector>

namespace lynx {
namespace base {
class PathUtils {
 public:
  // Convert relative path of local resources to absolute path
  static std::string RedirectUrlPath(const std::string &dirname,
                                     const std::string &url) {
    if (url.size() == 0) {
      return url;
    } else if (url.find("://") != std::string::npos) {
      return url;
    } else if (url[0] == '/') {
      return url;
    } else {
      std::stringstream input_url(dirname + url);
      std::vector<std::string> segment_urls;
      std::string current_segment;
      std::string output_url;
      while (getline(input_url, current_segment, '/')) {
        if (current_segment != "." && current_segment != "") {
          if (current_segment != "..") {
            segment_urls.push_back(current_segment);
          } else if (!segment_urls.empty()) {
            segment_urls.pop_back();
          }
        }
      }

      if (segment_urls.empty()) {
        return "/";
      } else {
        for (auto &segment_url : segment_urls) {
          output_url.append("/");
          output_url.append(segment_url);
        }
        return output_url;
      }
    }
  }
  static std::string Url(const std::string &url) {
    return "url(\"" + url + "\")";
  }

  static std::string JoinPaths(std::initializer_list<std::string> components) {
    std::stringstream stream;
    size_t i = 0;
    const size_t size = components.size();
    for (const auto &component : components) {
      i++;
      stream << component;
      if (i != size) {
#if defined(OS_WIN)
        stream << "\\";
#else
        stream << "/";
#endif
      }
    }
    return stream.str();
  }

  /**brief: get last path form macro __FILE__ at compile time
   * The feature running at compile time will be enable only when passed const
   * expression as paraments
   */
  static constexpr const char *GetLastPath(const char *filename,
                                           int32_t length) {
    for (int32_t i = length; i > 0; --i) {
      if (filename[i] == '/' || filename[i] == '\\') {
        return filename + i + 1;
      }
    }

    return filename;
  }
};
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_PATH_UTILS_H_

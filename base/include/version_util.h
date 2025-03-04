// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VERSION_UTIL_H_
#define BASE_INCLUDE_VERSION_UTIL_H_

#include <cstdio>
#include <iostream>
#include <string>

#include "base/include/log/log_stream.h"

namespace lynx {
namespace base {

struct Version {
  explicit Version(const std::string& version) {
    std::sscanf(version.c_str(), "%d.%d.%d.%d", &major_, &minor_, &revision_,
                &build_);
    if (major_ < 0) major_ = 0;
    if (minor_ < 0) minor_ = 0;
    if (revision_ < 0) revision_ = 0;
    if (build_ < 0) build_ = 0;
  }

  constexpr Version(int major, int minor, int revision = 0, int build = 0)
      : major_(major), minor_(minor), revision_(revision), build_(build) {}

  bool operator<(const Version& other) const {
    // Compare major
    if (major_ < other.major_)
      return true;
    else if (major_ > other.major_)
      return false;

    // Compare minor
    if (minor_ < other.minor_)
      return true;
    else if (minor_ > other.minor_)
      return false;

    // Compare revision
    if (revision_ < other.revision_)
      return true;
    else if (revision_ > other.revision_)
      return false;

    // Compare build
    if (build_ < other.build_)
      return true;
    else if (build_ > other.build_)
      return false;

    return false;
  }

  bool operator==(const Version& other) const {
    return major_ == other.major_ && minor_ == other.minor_ &&
           revision_ == other.revision_ && build_ == other.build_;
  }

  bool operator!=(const Version& other) const { return !(*this == other); }

  bool operator>(const Version& other) const {
    if (*this == other) return false;
    return !(*this < other);
  }

  bool operator<=(const Version& other) const {
    if (*this == other) return true;
    return *this < other;
  }

  bool operator>=(const Version& other) const {
    if (*this == other) return true;
    return *this > other;
  }

  std::string ToString() const {
    std::string version;
    version.append(std::to_string(major_))
        .append(".")
        .append(std::to_string(minor_));

    if (revision_ == 0 && build_ == 0) {
      return version;
    }

    return version.append(".")
        .append(std::to_string(revision_))
        .append(".")
        .append(std::to_string(build_));
  }

  friend std::ostream& operator<<(std::ostream& stream,
                                  const Version& version) {
    return stream << version.ToString();
  }

  friend base::logging::LogStream& operator<<(base::logging::LogStream& stream,
                                              const Version& version) {
    std::ostringstream output_version;
    output_version << version;
    stream << output_version;
    return stream;
  }

  int Major() const { return major_; }
  int Minor() const { return minor_; }
  int Revision() const { return revision_; }
  int Build() const { return build_; }

 private:
  int major_ = 0, minor_ = 0, revision_ = 0, build_ = 0;
};

}  // namespace base
}  // namespace lynx
#endif  // BASE_INCLUDE_VERSION_UTIL_H_

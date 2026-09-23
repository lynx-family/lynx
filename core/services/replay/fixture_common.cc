// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_common.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>

namespace lynx {
namespace tasm {
namespace replay {
namespace {

std::string ReadFile(const std::string& path, size_t max_bytes) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return "";
  }
  stream.seekg(0, std::ios::end);
  const std::streamoff size = stream.tellg();
  if (size <= 0 || static_cast<uint64_t>(size) > max_bytes) {
    return "";
  }
  stream.seekg(0, std::ios::beg);
  std::string content(static_cast<size_t>(size), '\0');
  stream.read(content.data(), static_cast<std::streamsize>(size));
  if (!stream) {
    return "";
  }
  return content;
}

bool IsSafeAssetPath(const std::string& path) {
  // Keep drive- and scheme-like paths invalid even on hosts where ':' is an
  // ordinary filename character.
  if (path.empty() || path.front() == '/' ||
      path.find('\\') != std::string::npos ||
      path.find(':') != std::string::npos) {
    return false;
  }
  size_t start = 0;
  while (start <= path.size()) {
    size_t end = path.find('/', start);
    std::string component = path.substr(start, end - start);
    if (component.empty() || component == "." || component == "..") {
      return false;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return true;
}

}  // namespace

int64_t NormalizeDelayMs(double delay_ms) {
  if (!std::isfinite(delay_ms) || delay_ms <= 0) {
    return 0;
  }
  constexpr double kMaxDelayMs =
      static_cast<double>(std::numeric_limits<int64_t>::max());
  if (delay_ms >= kMaxDelayMs) {
    return std::numeric_limits<int64_t>::max();
  }
  return static_cast<int64_t>(delay_ms);
}

std::string ReadFixtureScript(const std::string& fixture_directory,
                              size_t max_bytes) {
  return ReadFile(fixture_directory + "/fixture.js", max_bytes);
}

std::string ReadFixtureAsset(const std::string& fixture_directory,
                             std::string path, size_t max_bytes) {
  constexpr char kAssetsPrefix[] = "assets/";
  if (path.rfind(kAssetsPrefix, 0) == 0) {
    path.erase(0, sizeof(kAssetsPrefix) - 1);
  }
  if (!IsSafeAssetPath(path)) {
    return "";
  }
  return ReadFile(fixture_directory + "/assets/" + path, max_bytes);
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

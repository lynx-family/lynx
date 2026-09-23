// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_FIXTURE_COMMON_H_
#define CORE_SERVICES_REPLAY_FIXTURE_COMMON_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace lynx {
namespace tasm {
namespace replay {

constexpr int64_t kDefaultFixtureTimeoutMs = 5000;
constexpr size_t kDefaultFixtureMemoryLimitBytes = 64 * 1024 * 1024;
constexpr size_t kDefaultFixtureMaxScriptBytes = 4 * 1024 * 1024;
constexpr size_t kDefaultFixtureMaxAssetBytes = 16 * 1024 * 1024;

struct FixtureRuntimeLimits {
  // Budget for each synchronous execution, including host callbacks and any
  // serialization performed within its execution scope.
  int64_t timeout_ms = kDefaultFixtureTimeoutMs;
  // Engine allocation budget for the lifetime of this fixture runtime.
  size_t memory_limit_bytes = kDefaultFixtureMemoryLimitBytes;

  // Zero does not disable a limit.
  bool IsValid() const { return timeout_ms > 0 && memory_limit_bytes > 0; }
};

// Non-finite/non-positive delays become zero; large finite delays saturate.
int64_t NormalizeDelayMs(double delay_ms);

// Reads from an already extracted fixture directory. Missing, empty, unreadable
// or oversized files return an empty string. A zero budget rejects every file.
std::string ReadFixtureScript(const std::string& fixture_directory,
                              size_t max_bytes = kDefaultFixtureMaxScriptBytes);

// Accepts paths relative to assets/, with an optional "assets/" prefix. Rejects
// absolute paths, schemes, backslashes and empty/dot path components. The
// caller must supply an extracted directory whose links cannot escape its asset
// root; this check validates path syntax, not filesystem links.
std::string ReadFixtureAsset(const std::string& fixture_directory,
                             std::string path,
                             size_t max_bytes = kDefaultFixtureMaxAssetBytes);

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_FIXTURE_COMMON_H_

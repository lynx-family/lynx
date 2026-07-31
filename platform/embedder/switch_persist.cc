// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/switch_persist.h"

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace embedder {

namespace {

uint64_t StableHash(const std::string& input) {
  constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
  constexpr uint64_t kFnvPrime = 1099511628211ull;

  uint64_t hash = kFnvOffsetBasis;
  for (unsigned char c : input) {
    hash ^= c;
    hash *= kFnvPrime;
  }
  return hash;
}

std::string HexEncode(uint64_t value) {
  static constexpr char kHexDigits[] = "0123456789abcdef";
  std::string encoded(16, '0');
  for (int i = 15; i >= 0; --i) {
    encoded[i] = kHexDigits[value & 0xf];
    value >>= 4;
  }
  return encoded;
}

std::string GetExecutablePathOrDefault() {
  char buffer[PATH_MAX] = {0};
  ssize_t length = ::readlink("/proc/self/exe", buffer, sizeof(buffer));
  if (length <= 0 || static_cast<size_t>(length) >= sizeof(buffer)) {
    return "Default";
  }
  return std::string(buffer, static_cast<size_t>(length));
}

std::string GetPersistDirectory() {
  const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  if (xdg_config_home != nullptr && xdg_config_home[0] != '\0') {
    return std::string(xdg_config_home) + "/lynx";
  }

  const char* home = std::getenv("HOME");
  if (home != nullptr && home[0] != '\0') {
    return std::string(home) + "/.config/lynx";
  }

  return ".lynx";
}

bool EnsureDirectory(const std::string& path) {
  if (path.empty()) {
    return false;
  }

  struct stat info;
  if (::stat(path.c_str(), &info) == 0) {
    return S_ISDIR(info.st_mode);
  }

  size_t separator = path.find_last_of('/');
  if (separator != std::string::npos && separator > 0 &&
      !EnsureDirectory(path.substr(0, separator))) {
    return false;
  }

  return ::mkdir(path.c_str(), 0700) == 0;
}

std::string GetPersistFilePath() {
  return GetPersistDirectory() + "/devtool_switches";
}

std::string GetScopedPersistFilePath() {
  static const std::string path =
      GetPersistFilePath() + "_" +
      HexEncode(StableHash(GetExecutablePathOrDefault()));
  return path;
}

using PersistedValues = std::unordered_map<std::string, bool>;

PersistedValues LoadPersistedValues(const std::string& path) {
  PersistedValues values;
  std::ifstream input(path);
  std::string line;
  while (std::getline(input, line)) {
    size_t pos = line.find('=');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= line.size()) {
      continue;
    }
    values[line.substr(0, pos)] = line[pos + 1] == '1';
  }
  return values;
}

bool SavePersistedValues(const std::string& path,
                         const PersistedValues& values) {
  std::string directory = GetPersistDirectory();
  if (!EnsureDirectory(directory)) {
    return false;
  }

  std::ofstream output(path, std::ios::out | std::ios::trunc);
  if (!output.is_open()) {
    return false;
  }

  for (const auto& entry : values) {
    output << entry.first << "=" << (entry.second ? '1' : '0') << "\n";
  }
  return output.good();
}

std::mutex& PersistMutex() {
  static std::mutex mutex;
  return mutex;
}

}  // namespace

bool SwitchPersist::SetValueToPersistent(const std::string& key, bool value) {
  std::lock_guard<std::mutex> lock(PersistMutex());
  PersistedValues values = LoadPersistedValues(GetScopedPersistFilePath());
  values[key] = value;
  return SavePersistedValues(GetScopedPersistFilePath(), values);
}

bool SwitchPersist::GetValueFromPersistent(const std::string& key,
                                           bool default_value) {
  {
    std::lock_guard<std::mutex> lock(PersistMutex());
    for (const std::string& path :
         {GetScopedPersistFilePath(), GetPersistFilePath()}) {
      PersistedValues values = LoadPersistedValues(path);
      auto it = values.find(key);
      if (it != values.end()) {
        return it->second;
      }
    }
  }

  // Keep the old process-local fallback for callers that still rely on
  // SetBoolLocalEnv/GetBoolEnv semantics.
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  return env.GetBoolEnv(key, default_value);
}
}  // namespace embedder
}  // namespace lynx

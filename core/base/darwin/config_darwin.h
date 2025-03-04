// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DARWIN_CONFIG_DARWIN_H_
#define CORE_BASE_DARWIN_CONFIG_DARWIN_H_

#include <optional>
#include <string>

namespace lynx {
namespace tasm {

class LynxConfigDarwin {
 public:
  LynxConfigDarwin() = delete;
  ~LynxConfigDarwin() = delete;

  static std::optional<std::string> stringFromExternalEnv(
      const std::string& key);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_BASE_DARWIN_CONFIG_DARWIN_H_

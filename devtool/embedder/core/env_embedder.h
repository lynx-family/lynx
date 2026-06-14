// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_ENV_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_ENV_EMBEDDER_H_

#include <string>

namespace lynx {
namespace devtool {

/**
 * @brief This is a cross - platform interface used to set switches.
 * It will have different implementations on different platforms.
 * This design decouples the devtool/embedder from the implementation end.
 */
class EnvEmbedder {
 public:
  /**
   * @deprecated DevTool setting callers should use platform-specific
   * DevToolSettings typed APIs instead. This generic string-switch API is
   * retained for backward compatibility.
   */
  static void SetSwitch(const std::string& key, bool value);

  /**
   * @deprecated DevTool setting callers should use platform-specific
   * DevToolSettings typed APIs instead. This generic string-switch API is
   * retained for backward compatibility.
   */
  static bool GetSwitch(const std::string& key);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_ENV_EMBEDDER_H_

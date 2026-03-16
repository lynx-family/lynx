// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_UPDATE_MODE_H_
#define CORE_SHELL_UPDATE_MODE_H_

namespace lynx {
namespace shell {

/**
 * @enum LynxUpdateMode
 * @brief Defines the update mode for template data updates.
 *
 * This enum specifies how new data should be applied to the existing
 * template state. Different modes provide different behaviors for
 * data merging and lifecycle handling.
 */
enum class LynxUpdateMode : int32_t {
  /**
   * @brief Incremental update mode.
   *
   * Data will be incrementally merged with the previous data.
   * This is the default mode for partial updates without
   * destroying the existing runtime state.
   */
  UPDATE = 0,

  /**
   * @brief Reset update mode.
   *
   * Previous data will be cleared before applying the incoming data.
   * This mode performs a full data reset while maintaining the
   * existing runtime lifecycle.
   */
  RESET = 1,

  /**
   * @brief Reload update mode.
   *
   * Similar to RESET mode in terms of data handling, but additionally
   * triggers the destruction and recreation of the JavaScript runtime
   * lifecycle. This is useful when a complete runtime refresh is needed.
   */
  RELOAD = 2,
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_UPDATE_MODE_H_

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_DEVTOOL_STATE_H_
#define CORE_RENDERER_UTILS_DEVTOOL_STATE_H_

namespace lynx {
namespace tasm {

/**
 * Represents the lifecycle states of the DevTool.
 *
 * State Transition Graph:
 *
 * [UNAVAILABLE]
 *      |
 *      | (OnAttached)
 *      v
 *   [ATTACHED] <------------------------------------+
 *      |                                            |
 *      | (OnEnabled)                                |
 *      v                                            |
 *    [ENABLED]                                      |
 *      |                                            |
 *      | (OnInitialized)                            |
 *      v                                            |
 *   [INITIALIZED] <----------+                      | (OnDisabled)
 *      |                     |                      |
 *      | (OnConnected)       | (OnDisconnected)     |
 *      v                     |                      |
 *  [CONNECTED]  -------------+                      |
 *      |                                            |
 *      +--------------------------------------------+
 */
enum class DevToolState {
  /**
   * DevTool is not integrated into the application.
   */
  UNAVAILABLE = 0,

  /**
   * DevTool is present, but disabled by app policy or user switch.
   */
  ATTACHED,

  /**
   * DevTool is enabled by app policy or user switch, waiting for
   * initialization.
   */
  ENABLED,

  /**
   * DevTool environment is initialized (e.g. all necessary native libraries
   * loaded) and ready to connect.
   */
  INITIALIZED,

  /**
   * Active inspection or debugging session in progress.
   */
  CONNECTED
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_DEVTOOL_STATE_H_

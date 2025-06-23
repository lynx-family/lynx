// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PAGE_OPTIONS_H_
#define CORE_PUBLIC_PAGE_OPTIONS_H_

namespace lynx {
namespace tasm {

/**
 * Embedded mode is an experimental switch
 * When embeddedMode is set, we offer optimal performance for embedded
 * scenarios. But it will restrict business flexibility. Embedded mode
 * configuration options using bitwise operations for multiple selections
 *
 * Usage:
 * 1. Basic usage:
 *    - Use UNSET for no options selected
 *    - Use EMBEDDED_MODE_BASE for basic optimizations
 *    - Use EMBEDDED_MODE_ALL for all optimizations
 *
 * 2. Combine options:
 *    - Use bitwise OR (|) to combine options
 *    - Example: EMBEDDED_MODE_BASE | ENGINE_POOL
 *
 * 3. Check options:
 *    - Use bitwise AND (&) to check if an option is enabled
 *    - Example: (mode & ENGINE_POOL) != 0
 */
enum EmbeddedMode {
  /**
   * No optimization options selected
   */
  UNSET = 0,

  /**
   * Basic embedded mode with minimal optimizations
   */
  EMBEDDED_MODE_BASE = 1 << 0,

  /**
   * Engine pool optimization for better instance reuse
   */
  ENGINE_POOL = 1 << 1,

  /**
   *  Integrate Layout with Element
   */
  LAYOUT_IN_ELEMENT = 1 << 2,

  /**
   * Combination of all optimization options
   *
   * Note: When adding new optimization options, update this value
   */
  EMBEDDED_MODE_ALL = EMBEDDED_MODE_BASE | ENGINE_POOL | LAYOUT_IN_ELEMENT
};

/// Common options shared by components within a Lynx page.
/// Unlike PageConfig, the options are dynamic and can be updated on-the-flight
/// by calling LynxShell::SetPageOptions
struct PageOptions {
  static constexpr int32_t kUnknownInstanceID = -1;

  PageOptions() = default;

  explicit PageOptions(int32_t instance_id) : instance_id_(instance_id) {}

  void SetInstanceID(int32_t instance_id) { instance_id_ = instance_id; }

  int32_t GetInstanceID() const { return instance_id_; }

  // Set long task monitoring explicitly disabled for this instance.
  // If true, long task monitoring will always be disabled.
  // If false, long task monitoring will respect the default behavior defined by
  // lynx::tasm::timing::LongTaskMonitor
  void SetLongTaskMonitorDisabled(bool disabled) {
    long_task_disabled_ = disabled;
  }

  // Get long task monitoring disabled status for this instance.
  bool GetLongTaskMonitorDisabled() const { return long_task_disabled_; }

  void SetEmbeddedMode(EmbeddedMode mode) { embedded_mode_ = mode; }

  EmbeddedMode GetEmbeddedMode() const { return embedded_mode_; }

  bool IsEmbeddedModeOn() const {
    return embedded_mode_ & EmbeddedMode::EMBEDDED_MODE_BASE;
  }

 private:
  int32_t instance_id_{kUnknownInstanceID};
  bool long_task_disabled_{false};
  EmbeddedMode embedded_mode_{UNSET};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_PAGE_OPTIONS_H_

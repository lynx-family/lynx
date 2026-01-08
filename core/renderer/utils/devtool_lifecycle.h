// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_DEVTOOL_LIFECYCLE_H_
#define CORE_RENDERER_UTILS_DEVTOOL_LIFECYCLE_H_

#include <atomic>
#include <mutex>
#include <string>

#include "base/include/no_destructor.h"
#include "core/base/lynx_export.h"
#include "core/renderer/utils/devtool_state.h"

namespace lynx {
namespace tasm {

namespace testing {
class DevToolLifecycleTest;
}

/**
 * Manages the state transitions for the DevTool.
 * Ensures valid transitions between states like UNAVAILABLE, ATTACHED,
 * INITIALIZED, etc.
 */
class DevToolLifecycle {
 public:
  LYNX_EXPORT_FOR_DEVTOOL static DevToolLifecycle& GetInstance();

  // State checks
  LYNX_EXPORT_FOR_DEVTOOL bool IsAttached() const;
  LYNX_EXPORT_FOR_DEVTOOL bool IsInitialized() const;
  LYNX_EXPORT_FOR_DEVTOOL bool IsEnabled() const;
  LYNX_EXPORT_FOR_DEVTOOL bool IsConnected() const;
  // State checks end

  // Transitions
  /**
   * Called when the DevTool component is detected (e.g., via reflection).
   * Transition: UNAVAILABLE -> ATTACHED
   */
  void OnAttached();

  /**
   * Called when DevTool is enabled (by policy or user switch).
   * Transition: ATTACHED -> ENABLED
   */
  LYNX_EXPORT_FOR_DEVTOOL void OnEnabled();

  /**
   * Called when DevTool is disabled (by policy or user switch).
   * Transition: ENABLED/INITIALIZED/CONNECTED -> ATTACHED
   */
  LYNX_EXPORT_FOR_DEVTOOL void OnDisabled();

  /**
   * Called when the DevTool env is initialized, i.e. native library is loaded.
   * Transition: ENABLED -> INITIALIZED
   */
  void OnInitialized();

  /**
   * Called when a DevTool client connects.
   * Transition: INITIALIZED -> CONNECTED
   */
  LYNX_EXPORT_FOR_DEVTOOL void OnConnected();

  /**
   * Called when a DevTool client disconnects.
   * Transition: CONNECTED -> INITIALIZED
   */
  void OnDisconnected();
  // Transitions end

  // State synchronization
  class Delegate {
    /**
     * Interface for handling platform-specific state synchronization.
     *
     * We use the Delegate pattern here to decouple the Core layer from Platform
     * implementations (Android/iOS/etc.). This allows:
     * 1. Dependency Inversion: Core logic doesn't depend on JNI/Obj-C calls.
     * 2. Testability: Easy to mock for unit tests without platform
     * dependencies.
     * 3. Portability: Keeps the core C++ clean and platform-agnostic.
     */
   public:
    virtual ~Delegate() = default;
    virtual void SyncStateToPlatform(DevToolState state) = 0;
  };

  /**
   * Set the delegate for platform-specific state synchronization.
   */
  void SetDelegate(Delegate* delegate);

  /**
   * Synchronize state from platform (e.g. Android/iOS).
   * This is used when the lifecycle is driven by the platform side.
   *
   * Note: This method is NOT part of the Delegate interface because Delegate
   * handles Core -> Platform communication. This method handles the reverse
   * (Platform -> Core), acting as an entry point for the platform to push
   * updates.
   */
  void SyncStateFromPlatform(DevToolState state);

 private:
  // Friend the NoDestructor to allow access to the private constructor and
  // private destructor.
  friend class base::NoDestructor<DevToolLifecycle>;
  // Friend the test fixture to allow access to ResetForTesting.
  friend class testing::DevToolLifecycleTest;

  DevToolLifecycle() = default;
  ~DevToolLifecycle() = default;
  DevToolState GetState() const;
  void ResetForTesting();

  mutable std::mutex mutex_;
  std::atomic<Delegate*> delegate_{nullptr};
  DevToolState state_{DevToolState::UNAVAILABLE};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_DEVTOOL_LIFECYCLE_H_

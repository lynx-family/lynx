// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/devtool_lifecycle.h"

#include "core/renderer/utils/devtool_state.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class DevToolLifecycleTest : public ::testing::Test {
 public:
  DevToolLifecycleTest() = default;
  ~DevToolLifecycleTest() override = default;

 protected:
  void SetUp() override { DevToolLifecycle::GetInstance().ResetForTesting(); }

  DevToolState GetState(const DevToolLifecycle& lifecycle) {
    return lifecycle.GetState();
  }
};

TEST_F(DevToolLifecycleTest, StateOrdinality) {
  EXPECT_GT(DevToolState::ATTACHED, DevToolState::UNAVAILABLE);
  EXPECT_GT(DevToolState::ENABLED, DevToolState::ATTACHED);
  EXPECT_GT(DevToolState::INITIALIZED, DevToolState::ENABLED);
  EXPECT_GT(DevToolState::CONNECTED, DevToolState::INITIALIZED);
}

TEST_F(DevToolLifecycleTest, InitialState) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  EXPECT_EQ(GetState(lifecycle), DevToolState::UNAVAILABLE);
  EXPECT_FALSE(lifecycle.IsAttached());
  EXPECT_FALSE(lifecycle.IsEnabled());
  EXPECT_FALSE(lifecycle.IsInitialized());
  EXPECT_FALSE(lifecycle.IsConnected());
}

TEST_F(DevToolLifecycleTest, OnAttached) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  EXPECT_EQ(GetState(lifecycle), DevToolState::ATTACHED);
  EXPECT_TRUE(lifecycle.IsAttached());
  EXPECT_FALSE(lifecycle.IsEnabled());
}

TEST_F(DevToolLifecycleTest, OnEnabled) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  lifecycle.OnEnabled();
  EXPECT_EQ(GetState(lifecycle), DevToolState::ENABLED);
  EXPECT_TRUE(lifecycle.IsEnabled());
  EXPECT_FALSE(lifecycle.IsInitialized());
}

TEST_F(DevToolLifecycleTest, OnInitialized) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  lifecycle.OnEnabled();
  lifecycle.OnInitialized();
  EXPECT_EQ(GetState(lifecycle), DevToolState::INITIALIZED);
  EXPECT_TRUE(lifecycle.IsInitialized());
  EXPECT_FALSE(lifecycle.IsConnected());
}

TEST_F(DevToolLifecycleTest, OnConnected) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  lifecycle.OnEnabled();
  lifecycle.OnInitialized();
  lifecycle.OnConnected();
  EXPECT_EQ(GetState(lifecycle), DevToolState::CONNECTED);
  EXPECT_TRUE(lifecycle.IsConnected());
}

TEST_F(DevToolLifecycleTest, OnDisconnected) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  lifecycle.OnEnabled();
  lifecycle.OnInitialized();
  lifecycle.OnConnected();

  lifecycle.OnDisconnected();
  EXPECT_EQ(GetState(lifecycle), DevToolState::INITIALIZED);
  EXPECT_TRUE(lifecycle.IsInitialized());
  EXPECT_FALSE(lifecycle.IsConnected());
}

TEST_F(DevToolLifecycleTest, OnDisabled) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  lifecycle.OnEnabled();
  lifecycle.OnInitialized();

  lifecycle.OnDisabled();
  EXPECT_EQ(GetState(lifecycle), DevToolState::ATTACHED);

  // Re-enable to check if we can go back
  lifecycle.OnEnabled();
  lifecycle.OnInitialized();
  lifecycle.OnConnected();
  lifecycle.OnDisabled();
  EXPECT_EQ(GetState(lifecycle), DevToolState::ATTACHED);
}

TEST_F(DevToolLifecycleTest, InvalidTransitions) {
  auto& lifecycle = DevToolLifecycle::GetInstance();

  // Try enabling before attached
  lifecycle.OnEnabled();
  EXPECT_EQ(GetState(lifecycle), DevToolState::UNAVAILABLE);

  // Attach
  lifecycle.OnAttached();

  // Try initializing before enabled
  lifecycle.OnInitialized();
  EXPECT_EQ(GetState(lifecycle), DevToolState::ATTACHED);

  // Enable
  lifecycle.OnEnabled();

  // Try connecting before initialized
  lifecycle.OnConnected();
  EXPECT_EQ(GetState(lifecycle), DevToolState::ENABLED);
}

TEST_F(DevToolLifecycleTest, SyncStateFromPlatform) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  lifecycle.SyncStateFromPlatform(DevToolState::CONNECTED);
  EXPECT_EQ(GetState(lifecycle), DevToolState::CONNECTED);
  EXPECT_TRUE(lifecycle.IsConnected());

  lifecycle.SyncStateFromPlatform(DevToolState::UNAVAILABLE);
  EXPECT_EQ(GetState(lifecycle), DevToolState::UNAVAILABLE);
}

class MockDelegate : public DevToolLifecycle::Delegate {
 public:
  void SyncStateToPlatform(DevToolState state) override {
    last_state_ = state;
    call_count_++;
  }
  DevToolState last_state_ = DevToolState::UNAVAILABLE;
  int call_count_ = 0;
};

TEST_F(DevToolLifecycleTest, DelegateCalled) {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  MockDelegate delegate;
  lifecycle.SetDelegate(&delegate);

  lifecycle.OnAttached();
  EXPECT_EQ(delegate.call_count_, 1);
  EXPECT_EQ(delegate.last_state_, DevToolState::ATTACHED);

  lifecycle.OnEnabled();
  EXPECT_EQ(delegate.call_count_, 2);
  EXPECT_EQ(delegate.last_state_, DevToolState::ENABLED);

  // SyncStateFromPlatform should NOT call delegate
  lifecycle.SyncStateFromPlatform(DevToolState::INITIALIZED);
  EXPECT_EQ(delegate.call_count_, 2);

  lifecycle.OnConnected();
  EXPECT_EQ(delegate.call_count_, 3);
  EXPECT_EQ(delegate.last_state_, DevToolState::CONNECTED);

  // Cleanup
  lifecycle.SetDelegate(nullptr);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

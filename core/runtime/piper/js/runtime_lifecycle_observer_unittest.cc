// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/public/vsync_observer_interface.h"
#include "core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"
#include "core/runtime/piper/js/runtime_lifecycle_observer_impl.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/napi.h"
#include "third_party/napi/include/primjs_napi_defines.h"
#else
#include "third_party/binding/napi/shim/shim_napi.h"
#endif

namespace lynx {
namespace runtime {
namespace testing {
using VsyncParamType = base::MoveOnlyClosure<void, int64_t, int64_t>;
class MockVSyncObserver : public runtime::IVSyncObserver {
 public:
  MOCK_METHOD(void, RequestAnimationFrame, (uintptr_t, VsyncParamType),
              (override));

  MOCK_METHOD(void, RequestBeforeAnimationFrame, (uintptr_t, VsyncParamType),
              (override));

  MOCK_METHOD(void, RegisterAfterAnimationFrameListener, (VsyncParamType),
              (override));
};

class MockListener : public RuntimeLifecycleListenerDelegate {
 public:
  MockListener(RuntimeLifecycleListenerDelegate::DelegateType type)
      : RuntimeLifecycleListenerDelegate(type) {}
  ~MockListener() override = default;
  MOCK_METHOD(void, OnRuntimeCreate,
              ((std::shared_ptr<runtime::IVSyncObserver>)), (override));
  MOCK_METHOD(void, OnRuntimeInit, (int64_t), (override));
  MOCK_METHOD(void, OnAppEnterForeground, (), (override));
  MOCK_METHOD(void, OnAppEnterBackground, (), (override));
  MOCK_METHOD(void, OnRuntimeDetach, (), (override));
  void OnRuntimeAttach(Napi::Env env) override { is_attach_called = true; }

  bool is_attach_called{false};
};

TEST(RuntimeLifecycleObserver, FullListener) {
  auto observer = std::make_unique<RuntimeLifecycleObserverImpl>();
  std::shared_ptr<runtime::IVSyncObserver> mock_vsync =
      std::make_shared<MockVSyncObserver>();

  // add before event emit
  auto listener = std::make_unique<MockListener>(
      RuntimeLifecycleListenerDelegate::DelegateType::FULL);
  auto* listener_ptr = listener.get();
  EXPECT_CALL(*listener, OnRuntimeCreate(mock_vsync)).Times(1);
  EXPECT_CALL(*listener, OnRuntimeInit(1)).Times(1);
  EXPECT_CALL(*listener, OnRuntimeDetach()).Times(1);
  EXPECT_CALL(*listener, OnAppEnterForeground()).Times(1);
  EXPECT_CALL(*listener, OnAppEnterBackground()).Times(1);
  observer->AddEventListener(std::move(listener));

  observer->OnRuntimeInit(1);
  observer->OnRuntimeCreate(mock_vsync);
  observer->OnRuntimeAttach(nullptr);
  observer->OnRuntimeDetach();
  observer->OnAppEnterBackground();
  observer->OnAppEnterForeground();

  EXPECT_TRUE(listener_ptr->is_attach_called);

  // add after event emit
  auto listener_after = std::make_unique<MockListener>(
      RuntimeLifecycleListenerDelegate::DelegateType::FULL);
  auto* listener_after_ptr = listener_after.get();
  EXPECT_CALL(*listener_after, OnRuntimeCreate(mock_vsync)).Times(1);
  EXPECT_CALL(*listener_after, OnRuntimeInit(1)).Times(1);
  EXPECT_CALL(*listener_after, OnRuntimeDetach()).Times(1);
  EXPECT_CALL(*listener_after, OnAppEnterForeground()).Times(1);
  EXPECT_CALL(*listener_after, OnAppEnterBackground()).Times(1);
  observer->AddEventListener(std::move(listener_after));
  EXPECT_TRUE(listener_after_ptr->is_attach_called);
}

TEST(RuntimeLifecycleObserver, PartListener) {
  auto observer = std::make_unique<RuntimeLifecycleObserverImpl>();
  std::shared_ptr<runtime::IVSyncObserver> mock_vsync =
      std::make_shared<MockVSyncObserver>();

  // add before event emit
  auto listener = std::make_unique<MockListener>(
      RuntimeLifecycleListenerDelegate::DelegateType::PART);
  auto* listener_ptr = listener.get();
  EXPECT_CALL(*listener, OnRuntimeCreate(mock_vsync)).Times(0);
  EXPECT_CALL(*listener, OnRuntimeInit(1)).Times(0);
  EXPECT_CALL(*listener, OnRuntimeDetach()).Times(1);
  EXPECT_CALL(*listener, OnAppEnterForeground()).Times(0);
  EXPECT_CALL(*listener, OnAppEnterBackground()).Times(0);
  observer->AddEventListener(std::move(listener));

  observer->OnRuntimeInit(1);
  observer->OnRuntimeCreate(mock_vsync);
  observer->OnRuntimeAttach(nullptr);
  observer->OnRuntimeDetach();
  observer->OnAppEnterBackground();
  observer->OnAppEnterForeground();

  EXPECT_TRUE(listener_ptr->is_attach_called);

  // add after event emit
  auto listener_after = std::make_unique<MockListener>(
      RuntimeLifecycleListenerDelegate::DelegateType::PART);
  auto* listener_after_ptr = listener_after.get();
  EXPECT_CALL(*listener_after, OnRuntimeCreate(mock_vsync)).Times(0);
  EXPECT_CALL(*listener_after, OnRuntimeInit(1)).Times(0);
  EXPECT_CALL(*listener_after, OnRuntimeDetach()).Times(1);
  EXPECT_CALL(*listener_after, OnAppEnterForeground()).Times(0);
  EXPECT_CALL(*listener_after, OnAppEnterBackground()).Times(0);
  observer->AddEventListener(std::move(listener_after));
  EXPECT_TRUE(listener_after_ptr->is_attach_called);
}

}  // namespace testing
}  // namespace runtime
}  // namespace lynx

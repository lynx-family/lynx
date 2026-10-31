// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_realm_manager.h"

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "core/runtime/js/js_executor.h"
#include "core/runtime/js/jsi/quickjs/quickjs_api.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime_wrapper.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace {

class TrackedRealm : public SingleJSRealm {
 public:
  TrackedRealm(std::shared_ptr<js::JSIContext> context, int& destroyed)
      : SingleJSRealm(std::move(context)), destroyed_(destroyed) {}
  ~TrackedRealm() override { ++destroyed_; }

 private:
  int& destroyed_;
};

}  // namespace

class JSRealmManagerTest : public ::testing::Test {
 protected:
  void SetUp() override { manager_ = JSRealmManager::Instance(); }

  void TearDown() override {
    EXPECT_EQ(manager_->GetSharedRealm(kGroupId, false), nullptr);
    EXPECT_EQ(manager_->GetSharedRealm(kGroupId, true), nullptr);
  }

  base::UnsafeWeakPtr<JSRealm> AddRealm(
      int& destroyed, bool shared_vm, std::shared_ptr<js::JSIContext> context) {
    base::UnsafeOwningPtr<JSRealm> realm =
        base::MakeUnsafeOwning<TrackedRealm>(std::move(context), destroyed);
    auto weak = realm.GetWeakPtr();
    auto& realms = shared_vm ? manager_->shared_vm_realm_map_
                             : manager_->shared_realm_map_;
    realms.emplace(kGroupId,
                   JSRealmManager::SharedRealmEntry{std::move(realm), 0});
    return weak;
  }

  void Retain(bool shared_vm) {
    manager_->RetainSharedRealm(kGroupId, shared_vm);
  }

  void Release(bool shared_vm) {
    manager_->ReleaseSharedRealm(kGroupId, shared_vm);
  }

  void Attach(js::JSExecutor& executor, int& local_destroyed, bool shared_vm) {
    base::UnsafeOwningPtr<JSRealm> local_realm;
    if (shared_vm) {
      local_realm =
          base::MakeUnsafeOwning<TrackedRealm>(nullptr, local_destroyed);
    }
    Retain(shared_vm);
    executor.realm_state_ = std::make_unique<JSRealmState>();
    executor.realm_state_->local_realm = std::move(local_realm);
    executor.realm_state_->sharing = shared_vm
                                         ? JSRealmState::Sharing::kVM
                                         : JSRealmState::Sharing::kContext;
  }

  void AttachLocal(js::JSExecutor& executor, int& destroyed) {
    executor.realm_state_ = std::make_unique<JSRealmState>();
    executor.realm_state_->local_realm =
        base::MakeUnsafeOwning<TrackedRealm>(nullptr, destroyed);
    executor.realm_state_->sharing = JSRealmState::Sharing::kNone;
  }

  void CheckQuickJSHighMemory(bool shared_vm, bool initially_shared) {
    using tasm::performance::MemoryMonitor;
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    struct RestoreSettings {
      MemoryMonitor::Settings& settings =
          const_cast<MemoryMonitor::Settings&>(MemoryMonitor::GetSettings());
      MemoryMonitor::Settings saved = settings;
      ~RestoreSettings() { settings = saved; }
    } config;
    config.settings.global.enabled = 0;
    config.settings.global.large_vm_threshold_mb = 0;
    auto runtime = js::makeQuickJsRuntime();
    js::JSRuntimeExternalParams params;
    params.group_id = initially_shared ? kGroupId : "-1";
    auto context = manager_->CreateJSIContext(*runtime, params);
    auto engine =
        std::static_pointer_cast<js::QuickjsRuntimeInstance>(context->getVM());
    void* allocation = lepus_malloc_rt(engine->Runtime(), 2 * 1024 * 1024,
                                       ALLOC_TAG_WITHOUT_PTR);
    ASSERT_NE(allocation, nullptr);
    config.settings.global.large_vm_threshold_mb = 1;
    std::vector<lepus::Value> events;
    base::NotificationCallback callback(
        tasm::performance::kHighMemoryEventNotification,
        [&](const std::string&, intptr_t data) {
          events.push_back(*reinterpret_cast<const lepus::Value*>(data));
        });
    int destroyed = 0;
    if (initially_shared) {
      AddRealm(destroyed, shared_vm, context);
      Retain(shared_vm);
    }
    const auto heap_size = LEPUS_GetHeapSize(engine->Runtime());
    engine->OnGC("{}");  // Any existing notification can detect high memory.
    auto drain = [&] {
      fml::AutoResetWaitableEvent barrier;
      tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
          [&barrier] { barrier.Signal(); });
      barrier.Wait();
    };
    drain();
    EXPECT_EQ(events.size(), initially_shared ? 1u : 0u);
    engine->OnGC("{is_full_gc}");
    drain();
    EXPECT_EQ(events.size(), initially_shared ? 1u : 0u);
    engine->OnGC("{is_full_gc}");
    drain();
    EXPECT_EQ(events.size(), initially_shared ? 1u : 0u);
    if (!events.empty()) {
      const auto& event = events.front();
      EXPECT_EQ(event.GetProperty("type").StdString(), "large_vm");
      EXPECT_EQ(event.GetProperty("key").StdString(), kGroupId);
      EXPECT_EQ(event.GetProperty("bytes").UInt64(),
                static_cast<uint64_t>(heap_size));
      EXPECT_EQ(event.GetProperty("vm_type").StdString(), "quickjs");
      EXPECT_TRUE(event.GetProperty("slots").IsNil());
      EXPECT_TRUE(event.GetProperty("total_page_count").IsNil());
    }
    if (initially_shared) {
      Release(shared_vm);
    }
    lepus_free_rt(engine->Runtime(), allocation);
    context.reset();
    engine.reset();
  }

  static constexpr const char* kGroupId = "js-realm-manager-test";
  JSRealmManager* manager_ = nullptr;
};

TEST_F(JSRealmManagerTest, LastExecutorReleasesRealmDespiteExternalContextRef) {
  int destroyed = 0;
  auto context = std::make_shared<js::JSIContext>(nullptr);
  auto weak = AddRealm(destroyed, false, context);
  Retain(false);
  Retain(false);
  Release(false);
  EXPECT_FALSE(weak.Expired());
  EXPECT_EQ(destroyed, 0);
  Release(false);
  EXPECT_TRUE(weak.Expired());
  EXPECT_EQ(destroyed, 1);
  // Keeping a JSI context alive must not keep its realm/global state alive.
  EXPECT_NE(context, nullptr);
}

TEST_F(JSRealmManagerTest, QuickJSHighMemoryUsesLegacyGroupWithoutMonitoring) {
  CheckQuickJSHighMemory(false, true);
}

TEST_F(JSRealmManagerTest,
       QuickJSHighMemoryUsesSharedVMGroupWithoutMonitoring) {
  CheckQuickJSHighMemory(true, true);
}

TEST_F(JSRealmManagerTest, QuickJSHighMemoryIgnoresStandaloneVM) {
  CheckQuickJSHighMemory(true, false);
}

TEST_F(JSRealmManagerTest, SharingModesWithSameGroupIdHaveSeparateLifetimes) {
  int legacy_destroyed = 0;
  int shared_vm_destroyed = 0;
  auto legacy = AddRealm(legacy_destroyed, false, nullptr);
  auto shared_vm = AddRealm(shared_vm_destroyed, true, nullptr);
  Retain(false);
  Retain(true);
  Release(false);
  EXPECT_TRUE(legacy.Expired());
  EXPECT_FALSE(shared_vm.Expired());
  Release(true);
  EXPECT_TRUE(shared_vm.Expired());
  EXPECT_EQ(legacy_destroyed, 1);
  EXPECT_EQ(shared_vm_destroyed, 1);
}

TEST_F(JSRealmManagerTest, ExecutorDestroyIsIdempotentForSharedVM) {
  int group_destroyed = 0;
  int local_destroyed = 0;
  AddRealm(group_destroyed, true, nullptr);
  {
    js::JSExecutor first(kGroupId, nullptr, nullptr);
    js::JSExecutor second(kGroupId, nullptr, nullptr);
    Attach(first, local_destroyed, true);
    Attach(second, local_destroyed, true);
    first.Destroy();
    first.Destroy();
    EXPECT_EQ(local_destroyed, 1);
    EXPECT_EQ(group_destroyed, 0);
    second.Destroy();
    EXPECT_EQ(local_destroyed, 2);
    EXPECT_EQ(group_destroyed, 1);
  }
  EXPECT_EQ(group_destroyed, 1);
  EXPECT_EQ(local_destroyed, 2);
}

TEST_F(JSRealmManagerTest, ExternalDestroyReleasesSharedRealm) {
  int destroyed = 0;
  int unused_local_destroyed = 0;
  AddRealm(destroyed, false, nullptr);
  {
    js::JSExecutor executor(kGroupId, nullptr, nullptr);
    Attach(executor, unused_local_destroyed, false);
    EXPECT_EQ(destroyed, 0);
    executor.Destroy();
    EXPECT_EQ(destroyed, 1);
  }
  EXPECT_EQ(destroyed, 1);
}

TEST_F(JSRealmManagerTest, ExecutorOwnsIndependentRealm) {
  int destroyed = 0;
  {
    js::JSExecutor executor("-1", nullptr, nullptr);
    AttachLocal(executor, destroyed);
    executor.Destroy();
    executor.Destroy();
    EXPECT_EQ(destroyed, 1);
  }
  EXPECT_EQ(destroyed, 1);
}

}  // namespace runtime
}  // namespace lynx

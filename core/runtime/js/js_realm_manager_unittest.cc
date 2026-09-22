// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_realm_manager.h"

#include "core/runtime/js/js_executor.h"
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
    executor.realm_state_ = {nullptr, std::move(local_realm),
                             shared_vm ? JSRealmState::Sharing::kVM
                                       : JSRealmState::Sharing::kContext};
  }

  void AttachLocal(js::JSExecutor& executor, int& destroyed) {
    executor.realm_state_ = {
        nullptr, base::MakeUnsafeOwning<TrackedRealm>(nullptr, destroyed),
        JSRealmState::Sharing::kNone};
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

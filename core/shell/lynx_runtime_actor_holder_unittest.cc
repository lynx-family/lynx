// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_runtime_actor_holder.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class LynxRuntimeActorHolderTest : public ::testing::Test {
 protected:
  LynxRuntimeActorHolderTest() = default;
  ~LynxRuntimeActorHolderTest() override = default;

  void SetUp() override {}
  void TearDown() override {}

  static constexpr uint32_t kRuntimeCounts = 10;

  static constexpr int64_t kReleaseDelayedTime = 2000;  // ms
};

TEST_F(LynxRuntimeActorHolderTest, HoldAndRelease) {
  std::unordered_map<int64_t, std::shared_ptr<LynxActor<runtime::LynxRuntime>>>
      actors;
  auto js_runner = base::TaskRunnerManufactor::GetJSRunner("");

  // hold serveral LynxRuntimes
  for (uint32_t i = 0; i < kRuntimeCounts; ++i) {
    auto runtime = std::make_unique<runtime::LynxRuntime>("", 0, nullptr, false,
                                                          "", false);
    int64_t id = runtime->GetRuntimeId();
    auto actor = std::make_shared<LynxActor<runtime::LynxRuntime>>(
        std::move(runtime), js_runner);

    actors.emplace(id, actor);

    js_runner->PostTask(
        [actor]() { LynxRuntimeActorHolder::GetInstance()->Hold(actor); });
  }

  // check if LynxRuntimes have not released
  for (const auto& [id, actor] : actors) {
    ASSERT_TRUE(actor->Impl() != nullptr);
  }

  // release LynxRuntimes
  for (const auto& [id, actor] : actors) {
    js_runner->PostTask([id = id, actor = actor]() {
      LynxRuntimeActorHolder::GetInstance()->Release(id);
    });
  }

  // wait for all LynxRuntimeActorHolder::Release to complete
  fml::AutoResetWaitableEvent arwe;
  js_runner->PostTask([&arwe]() { arwe.Signal(); });
  arwe.Wait();

  // check if LynxRuntimes have released
  for (const auto& [id, actor] : actors) {
    ASSERT_TRUE(actor->Impl() == nullptr);
  }
}

TEST_F(LynxRuntimeActorHolderTest, HoldAndDelayedRelease) {
  std::unordered_map<int64_t, std::shared_ptr<LynxActor<runtime::LynxRuntime>>>
      actors;
  auto js_runner = base::TaskRunnerManufactor::GetJSRunner("");

  // hold serveral LynxRuntimes
  for (uint32_t i = 0; i < kRuntimeCounts; ++i) {
    auto runtime = std::make_unique<runtime::LynxRuntime>("", 0, nullptr, false,
                                                          "", false);
    int64_t id = runtime->GetRuntimeId();
    auto actor = std::make_shared<LynxActor<runtime::LynxRuntime>>(
        std::move(runtime), js_runner);

    actors.emplace(id, actor);

    js_runner->PostTask(
        [actor]() { LynxRuntimeActorHolder::GetInstance()->Hold(actor); });
  }

  // check if LynxRuntimes have not released
  for (const auto& [id, actor] : actors) {
    ASSERT_TRUE(actor->Impl() != nullptr);
  }

  // delayred release LynxRuntimes
  for (const auto& [id, actor] : actors) {
    js_runner->PostTask([id = id, actor = actor]() {
      LynxRuntimeActorHolder::GetInstance()->PostDelayedRelease(id);
    });
  }

  // wait for all LynxRuntimeActorHolder::PostDelayedRelease to complete
  fml::AutoResetWaitableEvent arwe;
  js_runner->PostTask([&arwe]() { arwe.Signal(); });
  arwe.Wait();

  // wait for all tasks of delayed release to complete
  arwe.Reset();
  js_runner->PostTask([&arwe, js_runner]() {
    js_runner->PostDelayedTask(
        [&arwe]() { arwe.Signal(); },
        fml::TimeDelta::FromMilliseconds(kReleaseDelayedTime));
  });
  arwe.Wait();

  // check if LynxRuntimes have released
  for (const auto& [id, actor] : actors) {
    ASSERT_TRUE(actor->Impl() == nullptr);
  }
}

}  // namespace shell
}  // namespace lynx

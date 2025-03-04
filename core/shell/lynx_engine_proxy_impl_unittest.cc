// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_engine_proxy_impl.h"

#include <thread>

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/task_runner.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class MockTasmRunner : public fml::TaskRunner {
 public:
  MockTasmRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

// Test that calling the LynxEngineProxy's API when the LynxEngine is empty does
// not cause a crash.
class LynxEngineProxyImplTest : public ::testing::Test {
 protected:
  LynxEngineProxyImplTest() = default;
  ~LynxEngineProxyImplTest() override = default;

  void SetUp() override {
    engine_actor_ = std::make_shared<shell::LynxActor<shell::LynxEngine>>(
        nullptr, fml::MakeRefCounted<MockTasmRunner>());
    engine_proxy_ = std::make_unique<LynxEngineProxyImpl>(engine_actor_);
  }

  void TearDown() override {}

  std::shared_ptr<LynxActor<LynxEngine>> engine_actor_;
  std::unique_ptr<LynxEngineProxyImpl> engine_proxy_;
};

TEST_F(LynxEngineProxyImplTest, SendTouchEvent) {
  engine_proxy_->SendTouchEvent("test", 0, 0, 0, 0, 0, 0, 0);
}

TEST_F(LynxEngineProxyImplTest, SendTouchEventParams) {
  engine_proxy_->SendTouchEvent("test", pub::ValueImplLepus(lepus::Value()));
}

TEST_F(LynxEngineProxyImplTest, SendCustomEvent) {
  engine_proxy_->SendCustomEvent("test", 0, pub::ValueImplLepus(lepus::Value()),
                                 "test");
}

TEST_F(LynxEngineProxyImplTest, OnPseudoStatusChanged) {
  engine_proxy_->OnPseudoStatusChanged(0, 0, 0);
}

TEST_F(LynxEngineProxyImplTest, SendGestureEvent) {
  engine_proxy_->SendGestureEvent(1, 0, "test",
                                  pub::ValueImplLepus(lepus::Value()));
}

TEST_F(LynxEngineProxyImplTest, SendBubbleEvent) {
  engine_proxy_->SendBubbleEvent("test", 0,
                                 pub::ValueImplLepus(lepus::Value()));
}

TEST_F(LynxEngineProxyImplTest, EnableRasterAnimation) {
  engine_proxy_->EnableRasterAnimation();
}

}  // namespace shell
}  // namespace lynx

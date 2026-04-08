// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/testing/mock/lynx_devtool_mediator_mock.h"
#define protected public
#define private public

#include <cstddef>
#include <memory>

#include "core/runtime/common/lynx_console_helper.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_observer_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorLepusObserverImplTest : public ::testing::Test {
 public:
  InspectorLepusObserverImplTest() {}
  ~InspectorLepusObserverImplTest() override {}
  void SetUp() override {
    mediator_ = std::make_shared<lynx::testing::LynxDevToolMediatorMock>();
    observer_ = std::make_shared<InspectorLepusObserverImpl>(nullptr);
    observer_->SetDevToolMediator(mediator_);
  }

 private:
  std::shared_ptr<InspectorLepusObserverImpl> observer_;
  std::shared_ptr<lynx::testing::LynxDevToolMediatorMock> mediator_;
};

TEST_F(InspectorLepusObserverImplTest, ShouldFetchDebugInfo) {
  bool result = observer_->ShouldFetchDebugInfo();
  EXPECT_FALSE(result);

  DevToolConfig::SetFetchDebugInfo(true, false);
  result = observer_->ShouldFetchDebugInfo();
  EXPECT_FALSE(result);

  DevToolConfig::SetFetchDebugInfo(true, true);
  result = observer_->ShouldFetchDebugInfo();
  EXPECT_TRUE(result);

  DevToolConfig::SetFetchDebugInfo(false, true);
  DevToolConfig::SetStopAtEntry(true, false);
  result = observer_->ShouldFetchDebugInfo();
  EXPECT_FALSE(result);

  DevToolConfig::SetStopAtEntry(true, true);
  result = observer_->ShouldFetchDebugInfo();
  EXPECT_TRUE(result);

  DevToolConfig::SetStopAtEntry(false, true);
  result = observer_->ShouldFetchDebugInfo();
  EXPECT_TRUE(result);

  // reset for other tests
  DevToolConfig::SetStopAtEntry(false, false);
  DevToolConfig::SetFetchDebugInfo(false, true);
}

// The following 4 functions only test the case when debugger_wp_ is nullptr,
// the internal implementation is tested in
// inspector_lepus_debugger_impl_unittest.cc.
TEST_F(InspectorLepusObserverImplTest, GetDebugInfo) {
  std::string result = observer_->GetDebugInfo("test");
  EXPECT_EQ(result, "");
}

TEST_F(InspectorLepusObserverImplTest, SetDebugInfoUrl) {
  observer_->SetDebugInfoUrl("test-url", "test-name");
}

TEST_F(InspectorLepusObserverImplTest, OnInspectorInited) {
  observer_->OnInspectorInited(kKeyEngineLepus, "test", nullptr);
}

TEST_F(InspectorLepusObserverImplTest, OnContextDestroyed) {
  observer_->OnContextDestroyed("test");
}

TEST_F(InspectorLepusObserverImplTest, TakeOver) {
  std::shared_ptr<InspectorLepusObserverImpl> other_observer1;
  observer_->TakeOver(other_observer1);

  observer_->TakeOver(observer_);
}

TEST_F(InspectorLepusObserverImplTest, OnConsoleEvent) {
  std::string alog_mes = "alog message";
  std::string debug_mes = "debug message";
  std::string error_mes = "error message";
  std::string info_mes = "info message";
  std::string log_mes = "log message";
  std::string report_mes = "report message";
  std::string warn_mes = "warn message";

  observer_->SetConsolePostNeeded(false);

  observer_->OnConsoleEvent(runtime::ConsoleAlog, alog_mes);
  EXPECT_EQ(mediator_->message_.text_, "");
  EXPECT_EQ(mediator_->message_.level_, -2);
  EXPECT_EQ(mediator_->message_.timestamp_, -1);

  observer_->SetConsolePostNeeded(true);

  observer_->OnConsoleEvent(runtime::ConsoleAlog, alog_mes);
  EXPECT_EQ(mediator_->message_.text_, alog_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_ALOG);

  observer_->OnConsoleEvent(runtime::ConsoleDebug, debug_mes);
  EXPECT_EQ(mediator_->message_.text_, debug_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_INFO);

  observer_->OnConsoleEvent(runtime::ConsoleError, error_mes);
  EXPECT_EQ(mediator_->message_.text_, error_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_ERROR);

  observer_->OnConsoleEvent(runtime::ConsoleInfo, info_mes);
  EXPECT_EQ(mediator_->message_.text_, info_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_INFO);

  observer_->OnConsoleEvent(runtime::ConsoleLog, log_mes);
  EXPECT_EQ(mediator_->message_.text_, log_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_LOG);

  observer_->OnConsoleEvent(runtime::ConsoleReport, report_mes);
  EXPECT_EQ(mediator_->message_.text_, report_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_REPORT);

  observer_->OnConsoleEvent(runtime::ConsoleWarn, warn_mes);
  EXPECT_EQ(mediator_->message_.text_, warn_mes);
  EXPECT_EQ(mediator_->message_.level_, runtime::CONSOLE_LOG_WARNING);
}

TEST_F(InspectorLepusObserverImplTest, CopyMembersFrom) {
  auto other_mediator =
      std::make_shared<lynx::testing::LynxDevToolMediatorMock>();
  auto other_debugger =
      std::make_shared<InspectorLepusDebuggerImpl>(other_mediator);
  auto other_observer = other_debugger->GetInspectorLepusObserver();
  other_observer->SetDevToolMediator(other_mediator);
  other_observer->need_post_console_ = true;

  observer_->CopyMembersFrom(other_observer);
  EXPECT_EQ(observer_->need_post_console_, false);
  EXPECT_EQ(observer_->mediator_ptr_.lock(), mediator_);

  auto debugger = std::make_shared<InspectorLepusDebuggerImpl>(mediator_);
  debugger->SetPreExecute(true);
  observer_->debugger_wp_ = debugger;
  observer_->CopyMembersFrom(other_observer);
  EXPECT_EQ(observer_->need_post_console_, true);
  EXPECT_EQ(observer_->mediator_ptr_.lock(), other_mediator);
  EXPECT_EQ(other_mediator->lepus_debugger_, debugger);
  EXPECT_EQ(debugger->pre_execute_, false);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx

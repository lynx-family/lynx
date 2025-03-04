// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shared_data/lynx_white_board.h"

#include <utility>

#include "core/public/pub_value.h"
#include "core/shared_data/white_board_runtime_delegate.h"
#include "core/shared_data/white_board_tasm_delegate.h"
#include "core/shell/testing/mock_native_facade.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class LynxWhiteBoardTest : public ::testing::Test {
 public:
  LynxWhiteBoardTest() {}
  ~LynxWhiteBoardTest() override {}

  void SetUp() override {}
};

TEST_F(LynxWhiteBoardTest, SetAndGetSharedDataRule0) {
  WhiteBoard white_board;
  base::String key = base::String("name");
  lepus::Value value = lepus::Value("value");
  white_board.SetGlobalSharedData(key.c_str(),
                                  std::make_shared<pub::ValueImplLepus>(value));
  lepus::Value gotValue = pub::ValueUtils::ConvertValueToLepusValue(
      *white_board.GetGlobalSharedData(key.c_str()));
  ASSERT_EQ(gotValue, value);
}

TEST_F(LynxWhiteBoardTest, RegisterLepusSharedDataListenerRule0) {
  WhiteBoard white_board;
  base::String key = base::String("name");
  lepus::Value value = lepus::Value("value");
  base::MoveOnlyClosure<void, const pub::Value&> trigger_callback =
      [](const pub::Value& value) {

      };
  base::MoveOnlyClosure<void> remove_callback = []() {

  };

  WhiteBoardListener listener = {0, std::move(trigger_callback),
                                 std::move(remove_callback)};
  white_board.RegisterSharedDataListener(WhiteBoardStorageType::TYPE_LEPUS,
                                         key.c_str(), std::move(listener));
}

TEST_F(LynxWhiteBoardTest, RegisterJSSharedDataListenerRule0) {
  WhiteBoard white_board;
  base::String key = base::String("name");
  lepus::Value value = lepus::Value("value");
  piper::ApiCallBack callback;
  base::MoveOnlyClosure<void, const pub::Value&> trigger_callback =
      [](const pub::Value& value) {

      };
  base::MoveOnlyClosure<void> remove_callback = []() {

  };
  WhiteBoardListener listener = {0, std::move(trigger_callback),
                                 std::move(remove_callback)};

  white_board.RegisterSharedDataListener(WhiteBoardStorageType::TYPE_JS,
                                         key.c_str(), std::move(listener));
}

TEST(WhiteBoardTasmDelegateTest, TestCallLepusCallbackWithValue) {
  TemplateAssembler* tasm = nullptr;
  std::shared_ptr<WhiteBoard> white_board;

  WhiteBoardTasmDelegate delegate(tasm, white_board);

  lepus::Value closure;
  lepus::Value param;

  delegate.CallLepusCallbackWithValue(closure, param);
}

TEST(WhiteBoardTasmDelegateTest, TestCallJSApiCallbackWithValue) {
  TemplateAssembler* tasm = nullptr;
  std::shared_ptr<WhiteBoard> white_board;

  WhiteBoardTasmDelegate delegate(tasm, white_board);

  piper::ApiCallBack callback;
  lepus::Value param;

  delegate.CallJSApiCallbackWithValue(callback, param);
}

TEST(WhiteBoardTasmDelegateTest, TestRemoveJSApiCallback) {
  TemplateAssembler* tasm = nullptr;
  std::shared_ptr<WhiteBoard> white_board;

  WhiteBoardTasmDelegate delegate(tasm, white_board);

  piper::ApiCallBack callback;

  delegate.RemoveJSApiCallback(callback);
}

TEST(WhiteBoardTasmDelegateTest, TestCallPlatformCallbackWithValue) {
  TemplateAssembler* tasm = nullptr;
  std::shared_ptr<WhiteBoard> white_board;

  WhiteBoardTasmDelegate delegate(tasm, white_board);

  std::unique_ptr<lynx::shell::PlatformCallBack> func =
      std::make_unique<lynx::shell::PlatformCallBack>(
          [](const lepus::Value& v) {});
  auto callback =
      std::make_shared<lynx::shell::PlatformCallBackHolder>(std::move(func));
  lepus::Value value;

  delegate.CallPlatformCallbackWithValue(callback, value);
}

TEST(WhiteBoardRuntimeDelegateTest, CallJSApiCallbackWithValue) {
  std::shared_ptr<WhiteBoard> white_board;
  WhiteBoardRuntimeDelegate delegate(white_board);

  piper::ApiCallBack callback;
  lepus::Value param;

  delegate.CallJSApiCallbackWithValue(callback, param);
}

TEST(WhiteBoardRuntimeDelegateTest, RemoveJSApiCallback) {
  std::shared_ptr<WhiteBoard> white_board;
  WhiteBoardRuntimeDelegate delegate(white_board);

  piper::ApiCallBack callback;

  delegate.RemoveJSApiCallback(callback);
}

TEST(WhiteBoardRuntimeDelegateTest, CallPlatformCallbackWithValue) {
  std::shared_ptr<WhiteBoard> white_board;
  WhiteBoardRuntimeDelegate delegate(white_board);

  std::unique_ptr<lynx::shell::PlatformCallBack> func =
      std::make_unique<lynx::shell::PlatformCallBack>(
          [](const lepus::Value& v) {});
  auto callback =
      std::make_shared<lynx::shell::PlatformCallBackHolder>(std::move(func));
  lepus::Value value;

  delegate.CallPlatformCallbackWithValue(callback, value);
}

TEST(WhiteBoardRuntimeDelegateTest, RemovePlatformCallback) {
  std::shared_ptr<WhiteBoard> white_board;
  auto facade = std::make_unique<shell::MockNativeFacade>();
  auto js_task_runner = lynx::base::TaskRunnerManufactor::GetJSRunner("test");
  auto facade_actor = std::make_shared<shell::LynxActor<shell::NativeFacade>>(
      std::move(facade), js_task_runner, 0);
  WhiteBoardRuntimeDelegate delegate(white_board);
  delegate.SetRuntimeFacadeActor(facade_actor);

  std::unique_ptr<lynx::shell::PlatformCallBack> func =
      std::make_unique<lynx::shell::PlatformCallBack>(
          [](const lepus::Value& v) {});
  auto callback =
      std::make_shared<lynx::shell::PlatformCallBackHolder>(std::move(func));

  delegate.RemovePlatformCallback(callback);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

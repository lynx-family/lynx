// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/debug/lynx_error.h"

#include "base/include/debug/lynx_assert.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

static constexpr int kTestErrorCode = -100;
static constexpr char kTestErrorMessage[] = "Test error.";
static constexpr char kTestErrorSuggestion[] = "Some fix suggestion";
static constexpr char kTestErrorContextValue1[] = "context field test value1";
static constexpr char kTestErrorContextValue2[] = "context field test value2";

TEST(ErrorStorageTest, GetSetAndRest) {
  int err = -1;
  int err2 = -100;
  std::string err_msg = "Test error.";
  std::string err_msg2 = "Other error.";

  ErrorStorage::GetInstance().SetError(-1, "Test error.");
  auto& error = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error, nullptr);
  ASSERT_EQ(error->error_code_, err);
  ASSERT_EQ(error->error_message_, err_msg);

  // If not reset, ignore the following error
  ErrorStorage::GetInstance().SetError(err2, err_msg2);
  auto& error_set_again = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error_set_again, nullptr);
  ASSERT_EQ(error_set_again->error_code_, err);
  ASSERT_EQ(error_set_again->error_message_, err_msg);

  // Reset error
  ErrorStorage::GetInstance().Reset();
  auto& reset_error = ErrorStorage::GetInstance().GetError();
  ASSERT_EQ(reset_error, nullptr);

  // After reset, we can set error now.
  ErrorStorage::GetInstance().SetError(err2, err_msg2);
  auto& error2 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error2, nullptr);
  ASSERT_EQ(error2->error_code_, err2);
  ASSERT_EQ(error2->error_message_, err_msg2);
}

TEST(LynxErrorTest, MacroWithString) {
  int error_code = 601;
  constexpr char error_message[] = "some error occurred!";

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test report error by LynxInfo with string
  LynxInfo(error_code, error_message);
  auto& error1 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error1, nullptr);
  ASSERT_EQ(error1->error_code_, error_code);
  ASSERT_EQ(error1->error_message_, error_message);

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test report error by LynxWarning with expression value is true
  LynxWarning(true, error_code, error_message);
  auto& error3 = ErrorStorage::GetInstance().GetError();
  ASSERT_EQ(error3, nullptr);

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test report error by LynxWarning with expression value is false
  LynxWarning(false, error_code, error_message);
  auto& error4 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error4, nullptr);
  ASSERT_EQ(error4->error_code_, error_code);
  ASSERT_EQ(error4->error_message_, error_message);
}

TEST(LynxErrorTest, MacroWithFormatString) {
  int* ptr = new int(10);
  int error_code = 601;
  constexpr char fmt_error_message[] =
      "the error is %s, the code is %d, the pointer is %p";
  std::stringstream ss;
  ss << "the error is error, the code is 601, the pointer is " << ptr;
  std::string expect_fmt_error_msg = ss.str();

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test report error by LynxInfo with format string
  LynxInfo(error_code, fmt_error_message, "error", error_code, ptr);
  auto& error2 = ErrorStorage::GetInstance().GetError();
  // ASSERT_NE(error2, nullptr);
  ASSERT_EQ(error2->error_code_, error_code);
  ASSERT_EQ(error2->error_message_, expect_fmt_error_msg);

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test report error by LynxInfo with format string
  LynxWarning(false, error_code, fmt_error_message, "error", error_code, ptr);
  auto& error5 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error5, nullptr);
  ASSERT_EQ(error5->error_code_, error_code);
  ASSERT_EQ(error5->error_message_, expect_fmt_error_msg);
}

TEST(LynxErrorTest, StoreError) {
  int error_code = 601;
  constexpr char error_message[] = "some error occurred!";
  constexpr char fix_suggestion[] = "a fix suggestion";

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test store error by LYNX_ERROR
  LYNX_ERROR(error_code, error_message, fix_suggestion);
  auto& error1 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error1, nullptr);
  ASSERT_EQ(error1->error_code_, error_code);
  ASSERT_EQ(error1->error_message_, error_message);
  ASSERT_EQ(error1->error_level_, LynxErrorLevel::Error);

  // reset error
  ErrorStorage::GetInstance().Reset();

  LYNX_WARN(error_code, error_message, fix_suggestion);
  auto& error2 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error2, nullptr);
  ASSERT_EQ(error2->error_level_, LynxErrorLevel::Warn);

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test store error by LYNX_ERROR_CHECK with expression value is true
  LYNX_ERROR_CHECK(true, error_code, error_message, fix_suggestion);
  auto& error3 = ErrorStorage::GetInstance().GetError();
  ASSERT_EQ(error3, nullptr);

  // reset error
  ErrorStorage::GetInstance().Reset();

  // test store error by LYNX_ERROR_CHECK with expression value is false
  LYNX_ERROR_CHECK(false, error_code, error_message, fix_suggestion);
  auto& error4 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error4, nullptr);
  ASSERT_EQ(error4->error_code_, error_code);
  ASSERT_EQ(error4->error_message_, error_message);
  ASSERT_EQ(error4->error_level_, LynxErrorLevel::Error);

  // reset error
  ErrorStorage::GetInstance().Reset();

  LYNX_WARN_CHECK(false, error_code, error_message, fix_suggestion);
  auto& error5 = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error5, nullptr);
  ASSERT_EQ(error5->error_level_, LynxErrorLevel::Warn);
}

TEST(LynxErrorTest, AddCustomInfoToStoredError) {
  int err_code = -100;
  std::string err_msg = "Test error.";
  std::string fix_suggestion = "Some fix suggestion";

  // Test add custom info when ErrorStorage is empty
  ErrorStorage::GetInstance().Reset();
  std::unordered_map<std::string, std::string> custom_info(
      {{"key1", "value1"}, {"key2", "value2"}});
  ErrorStorage::GetInstance().AddCustomInfoToError(custom_info);
  ErrorStorage::GetInstance().AddCustomInfoToError("key3", "value3");
  ASSERT_EQ(ErrorStorage::GetInstance().GetError(), nullptr);

  // Test add custom info when ErrorStorage has error
  LynxError lynxError(err_code, err_msg, fix_suggestion, LynxErrorLevel::Error);
  ErrorStorage::GetInstance().SetError(std::move(lynxError));
  ASSERT_NE(ErrorStorage::GetInstance().GetError(), nullptr);
  ASSERT_TRUE(ErrorStorage::GetInstance().GetError()->custom_info_.empty());

  ErrorStorage::GetInstance().AddCustomInfoToError(custom_info);
  ErrorStorage::GetInstance().AddCustomInfoToError("key3", "value3");
  const auto& error = ErrorStorage::GetInstance().GetError();
  ASSERT_NE(error->custom_info_.find("key1"), error->custom_info_.end());
  ASSERT_NE(error->custom_info_.find("key2"), error->custom_info_.end());
  ASSERT_NE(error->custom_info_.find("key3"), error->custom_info_.end());
  ASSERT_EQ(error->custom_info_.find("key1")->second, "value1");
  ASSERT_EQ(error->custom_info_.find("key2")->second, "value2");
  ASSERT_EQ(error->custom_info_.find("key3")->second, "value3");
}

TEST(LynxErrorTest, AddContextInfo) {
  LynxError error{kTestErrorCode, std::string(kTestErrorMessage),
                  kTestErrorSuggestion, LynxErrorLevel::Error};
  error.AddContextInfo("key1", kTestErrorContextValue1);
  error.AddContextInfo("key2", kTestErrorContextValue2);
  ASSERT_NE(error.custom_info_.find("lynx_context_key1"),
            error.custom_info_.end());
  ASSERT_NE(error.custom_info_.find("lynx_context_key2"),
            error.custom_info_.end());
  ASSERT_EQ(error.custom_info_.find("lynx_context_key1")->second,
            kTestErrorContextValue1);
  ASSERT_EQ(error.custom_info_.find("lynx_context_key2")->second,
            kTestErrorContextValue2);
}

TEST(LynxErrorTest, GetLevelString) {
  int32_t fatal_int = static_cast<int32_t>(LynxErrorLevel::Fatal);
  int32_t error_int = static_cast<int32_t>(LynxErrorLevel::Error);
  int32_t warn_int = static_cast<int32_t>(LynxErrorLevel::Warn);
  int32_t unknown_int = 100;
  ASSERT_EQ(LynxError::GetLevelString(fatal_int), "fatal");
  ASSERT_EQ(LynxError::GetLevelString(error_int), "error");
  ASSERT_EQ(LynxError::GetLevelString(warn_int), "warn");
  ASSERT_EQ(LynxError::GetLevelString(unknown_int), "error");
}

}  // namespace base
}  // namespace lynx

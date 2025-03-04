// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/base_devtool/native/js_inspect/script_manager_ng.h"

#include "devtool/base_devtool/native/js_inspect/inspector_client_delegate_base_impl_unittest.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {
class ScriptManagerNGTest : public ::testing::Test {
 public:
  ScriptManagerNGTest() {}
  ~ScriptManagerNGTest() override {}
  void SetUp() override {
    script_manager_ = std::make_unique<ScriptManagerNG>();
    delegate_ = std::make_shared<MockInspectorClientDelegateBaseImpl>(
        kKeyEngineQuickjs);
  }

 private:
  std::unique_ptr<ScriptManagerNG> script_manager_;
  std::shared_ptr<MockInspectorClientDelegateBaseImpl> delegate_;
};

TEST_F(ScriptManagerNGTest, SetBreakpoint) {
  std::string empty_message = "{}";
  std::string set_breakpoint_message =
      "{\"id\":43,\"method\":\"Debugger.setBreakpointByUrl\",\"params\":{"
      "\"lineNumber\":32,\"url\":\"file://view3/"
      "app-service.js\",\"columnNumber\":16,\"condition\":\"this.data.count!=="
      "1\"},\"timestamp\":1724137090039}";
  std::string set_breakpoint_response_error_message_1 =
      "{\"method\":\"test\",\"params\":{}}";
  std::string set_breakpoint_response_error_message_2 =
      "{\"id\":42,\"result\":{}}";
  std::string set_breakpoint_response_message =
      "{\n   \"id\" : 43,\n   \"result\" : {\n      \"breakpointId\" : "
      "\"1:32:16:file://view3/app-service.js\",\n      \"locations\" : [\n     "
      "    {\n            \"columnNumber\" : 21,\n            \"lineNumber\" : "
      "32,\n            \"scriptId\" : \"34\"\n         }\n      ]\n   }\n}\n";

  rapidjson::Document json_mes;
  delegate_->ParseStrToJson(json_mes, empty_message);
  script_manager_->SetBreakpointDetail(json_mes);
  EXPECT_TRUE(script_manager_->GetBreakpoints().empty());

  json_mes.RemoveAllMembers();
  delegate_->ParseStrToJson(json_mes, set_breakpoint_message);
  script_manager_->SetBreakpointDetail(json_mes);

  json_mes.RemoveAllMembers();
  delegate_->ParseStrToJson(json_mes, set_breakpoint_response_error_message_1);
  script_manager_->SetBreakpointId(json_mes);
  EXPECT_TRUE(script_manager_->GetBreakpoints().empty());

  json_mes.RemoveAllMembers();
  delegate_->ParseStrToJson(json_mes, set_breakpoint_response_error_message_2);
  script_manager_->SetBreakpointId(json_mes);
  EXPECT_TRUE(script_manager_->GetBreakpoints().empty());

  json_mes.RemoveAllMembers();
  delegate_->ParseStrToJson(json_mes, set_breakpoint_response_message);
  script_manager_->SetBreakpointId(json_mes);
  EXPECT_TRUE(script_manager_->set_breakpoint_map_.empty());
  auto it = (script_manager_->GetBreakpoints()).begin();
  EXPECT_EQ(it->first, "1:32:16:file://view3/app-service.js");
  EXPECT_EQ(it->second.breakpoint_id_, "1:32:16:file://view3/app-service.js");
  EXPECT_EQ(it->second.line_number_, 32);
  EXPECT_EQ(it->second.column_number_, 16);
  EXPECT_EQ(it->second.url_, "file://view3/app-service.js");
  EXPECT_EQ(it->second.condition_, "this.data.count!==1");
}
}  // namespace testing
}  // namespace devtool
}  // namespace lynx

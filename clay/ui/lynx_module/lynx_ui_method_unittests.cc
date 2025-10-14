// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

class LynxUIMethodTest : public UITest {};

TEST_F_UI(LynxUIMethodTest, InvokeTest) {
  bool called = false;
  LynxUIMethodResult ret_code;
  auto callback = [&](LynxUIMethodResult code, clay::Value data) {
    called = true;
    ret_code = code;
  };

  auto& registar = clay::LynxUIMethodRegistrar::Instance();

  // Call none-exists mothod
  auto* view = new View(0, page_.get());
  LynxModuleValues values;
  bool ret = registar.Invoke("selectTab", view, values, callback);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(called);
  EXPECT_EQ(ret_code, LynxUIMethodResult::kMethodNotFound);
  called = false;

  // Call existing method on BaseView
  ret = registar.Invoke("setFocus", view, values, callback);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(called);
  EXPECT_EQ(ret_code, LynxUIMethodResult::kSuccess);
}

}  // namespace clay

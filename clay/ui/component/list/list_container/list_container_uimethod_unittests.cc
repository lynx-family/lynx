// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/ui/component/list/list_container/list_container_wrapper.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

class ListContainerUIMethodTest : public UITest {
 protected:
  void UISetUp() override {
    wrapper_ = std::make_unique<ListContainerWrapper>(1, page_.get());
    wrapper_->GetListContainerView()->SetBound(0, 0, 100, 100);
  }

  void UITearDown() override { wrapper_.reset(); }

  std::unique_ptr<ListContainerWrapper> wrapper_;
};

}  // namespace

TEST_F_UI(ListContainerUIMethodTest, GetScrollInfoReturnsCurrentSchema) {
  wrapper_->UpdateContentOffsetForListContainer(300, 0, 40);

  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "getScrollInfo", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        ASSERT_EQ(code, LynxUIMethodResult::kSuccess);
        ASSERT_TRUE(data.IsMap());
        const auto& result = data.GetMap();
        for (const char* key :
             {"scrollX", "scrollY", "maxScrollOffset", "scrollLeft",
              "scrollTop", "scrollWidth", "scrollHeight", "isDragging"}) {
          EXPECT_NE(result.find(key), result.end()) << key;
        }
        EXPECT_EQ(result.find("scrollRange"), result.end());
        EXPECT_FLOAT_EQ(result.at("scrollX").GetFloat(), 0);
        EXPECT_FLOAT_EQ(result.at("scrollY").GetFloat(), 40);
        EXPECT_FLOAT_EQ(result.at("maxScrollOffset").GetFloat(), 200);
        EXPECT_FLOAT_EQ(result.at("scrollLeft").GetFloat(), 0);
        EXPECT_FLOAT_EQ(result.at("scrollTop").GetFloat(), 40);
        EXPECT_FALSE(result.at("isDragging").GetBool());
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerUIMethodTest, GetVisibleCellsReturnsArray) {
  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "getVisibleCells", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kSuccess);
        EXPECT_TRUE(data.IsArray());
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerUIMethodTest, GetVisibleItemsPositionsReturnsArray) {
  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "getVisibleItemsPositions", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kSuccess);
        EXPECT_TRUE(data.IsArray());
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerUIMethodTest, GetScrollInfoClampsNegativeRangeToZero) {
  wrapper_->UpdateContentOffsetForListContainer(50, 0, 0);

  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "getScrollInfo", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        ASSERT_EQ(code, LynxUIMethodResult::kSuccess);
        ASSERT_TRUE(data.IsMap());
        EXPECT_FLOAT_EQ(data.GetMap().at("maxScrollOffset").GetFloat(), 0);
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerUIMethodTest, AutoScrollValidatesRate) {
  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "autoScroll", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kParamInvalid);
        ASSERT_TRUE(data.IsString());
        EXPECT_EQ(data.GetString(), "rate is required for autoScroll");
      });
  EXPECT_TRUE(callback_invoked);

  callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "autoScroll",
      {{"start", clay::Value(false)}, {"rate", clay::Value("60px")}},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kSuccess);
        EXPECT_TRUE(data.IsNone());
      });
  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerUIMethodTest, ScrollToPositionRequiresIndex) {
  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "scrollToPosition", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kParamInvalid);
        EXPECT_TRUE(data.IsNone());
      });

  EXPECT_TRUE(callback_invoked);
}

}  // namespace clay

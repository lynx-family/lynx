// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <memory>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/list/list_container/list_container_wrapper.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

class ListContainerWrapperTest : public UITest {
 protected:
  void UISetUp() override {
    wrapper_ = std::make_unique<ListContainerWrapper>(1, page_.get());
    wrapper_->GetListContainerView()->SetBound(0, 0, 100, 100);
  }

  void UITearDown() override { wrapper_.reset(); }

  std::unique_ptr<ListContainerWrapper> wrapper_;
};

TEST_F_UI(ListContainerWrapperTest, ScrollByRequiresOffset) {
  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "scrollBy", {},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value&) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kParamInvalid);
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerWrapperTest, ScrollByRejectsNonFiniteOffset) {
  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "scrollBy",
      {{"offset", clay::Value(std::numeric_limits<double>::infinity())}},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value&) {
        callback_invoked = true;
        EXPECT_EQ(code, LynxUIMethodResult::kParamInvalid);
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(ListContainerWrapperTest,
          ScrollByReturnsConsumedAndUnconsumedVerticalOffsets) {
  wrapper_->UpdateContentOffsetForListContainer(300, 0, 0);

  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "scrollBy", {{"offset", clay::Value(240)}},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        ASSERT_EQ(code, LynxUIMethodResult::kSuccess);
        ASSERT_TRUE(data.IsMap());
        const auto& result = data.GetMap();
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("consumedX")),
                        0.f);
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("consumedY")),
                        200.f);
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("unconsumedX")),
                        240.f);
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("unconsumedY")),
                        40.f);
      });

  EXPECT_TRUE(callback_invoked);
  EXPECT_FLOAT_EQ(wrapper_->GetListContainerView()->GetScrollOffset().y(),
                  200.f);
}

TEST_F_UI(ListContainerWrapperTest,
          ScrollByUsesHorizontalAxisForHorizontalListContainer) {
  wrapper_->SetAttribute("scroll-orientation", clay::Value("horizontal"));
  wrapper_->UpdateContentOffsetForListContainer(300, 0, 0);

  bool callback_invoked = false;
  InvokeUIMethod(
      wrapper_.get(), "scrollBy", {{"offset", clay::Value(40)}},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        ASSERT_EQ(code, LynxUIMethodResult::kSuccess);
        ASSERT_TRUE(data.IsMap());
        const auto& result = data.GetMap();
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("consumedX")),
                        40.f);
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("consumedY")),
                        0.f);
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("unconsumedX")),
                        0.f);
        EXPECT_FLOAT_EQ(attribute_utils::GetDouble(result.at("unconsumedY")),
                        40.f);
      });

  EXPECT_TRUE(callback_invoked);
  EXPECT_FLOAT_EQ(wrapper_->GetListContainerView()->GetScrollOffset().x(),
                  40.f);
}

}  // namespace clay

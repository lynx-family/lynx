// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_orientation_helper.h"

#include "core/list/decoupled_list_container_impl.h"
#include "core/list/testing/mock_list_element.h"
#include "core/list/testing/mock_list_item_element.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#undef protected
#undef private

namespace lynx {
namespace list {

class DecoupledListOrientationHelperTest : public ::testing::Test {
 public:
  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    mock_list_element_->width_ = 320.f;
    mock_list_element_->height_ = 480.f;
    mock_list_element_->paddings_ = {10.f, 20.f, 30.f, 40.f};
    mock_list_element_->borders_ = {1.f, 2.f, 3.f, 4.f};
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
  }

 protected:
  std::unique_ptr<ListOrientationHelper> CreateOrientationHelper(
      Orientation orientation) {
    return ListOrientationHelper::CreateListOrientationHelper(
        list_container_impl_->list_layout_manager(), orientation);
  }

  std::unique_ptr<ItemHolder> CreateItemHolder(Orientation orientation) {
    MockListItemElement item_element(1);
    item_element.width_ = 120.f;
    item_element.height_ = 80.f;
    item_element.margins_ = {11.f, 12.f, 13.f, 14.f};

    auto item_holder = std::make_unique<ItemHolder>(
        0, "item", list_container_impl_->list_animation_manager());
    item_holder->SetOrientation(orientation);
    item_holder->UpdateLayoutFromItemDelegate(&item_element);
    item_holder->SetLeft(100.f);
    item_holder->SetTop(200.f);
    item_holder->SetMainAxisGap(7.f);
    return item_holder;
  }

  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_;
  std::unique_ptr<MockListElement> mock_list_element_;
  std::unique_ptr<ListContainerImpl> list_container_impl_;
};

// Verifies that a vertical list reads container dimensions, borders, and
// padding along the vertical main axis and horizontal cross axis.
TEST_F(DecoupledListOrientationHelperTest, VerticalContainerGeometry) {
  auto helper = CreateOrientationHelper(Orientation::kVertical);

  EXPECT_TRUE(helper->IsVertical());
  EXPECT_FLOAT_EQ(helper->GetMeasurement(), 474.f);
  EXPECT_FLOAT_EQ(helper->GetMeasurementInOther(), 316.f);
  EXPECT_FLOAT_EQ(helper->GetMeasurementInOtherWithoutPadding(), 276.f);
  EXPECT_FLOAT_EQ(helper->GetStartAfterPadding(), 20.f);
  EXPECT_FLOAT_EQ(helper->GetEndAfterPadding(), 434.f);
  EXPECT_FLOAT_EQ(helper->GetStartAfterPaddingInOther(), 10.f);
  EXPECT_FLOAT_EQ(helper->GetEndPadding(), 40.f);
}

// Verifies that a horizontal list swaps the main and cross axes and uses
// horizontal padding to calculate the available layout range.
TEST_F(DecoupledListOrientationHelperTest, HorizontalContainerGeometry) {
  auto helper = CreateOrientationHelper(Orientation::kHorizontal);

  EXPECT_FALSE(helper->IsVertical());
  EXPECT_FLOAT_EQ(helper->GetMeasurement(), 316.f);
  EXPECT_FLOAT_EQ(helper->GetMeasurementInOther(), 474.f);
  EXPECT_FLOAT_EQ(helper->GetMeasurementInOtherWithoutPadding(), 414.f);
  EXPECT_FLOAT_EQ(helper->GetStartAfterPadding(), 10.f);
  EXPECT_FLOAT_EQ(helper->GetEndAfterPadding(), 286.f);
  EXPECT_FLOAT_EQ(helper->GetStartAfterPaddingInOther(), 20.f);
  EXPECT_FLOAT_EQ(helper->GetEndPadding(), 30.f);
}

// Verifies that vertical item geometry includes the main-axis gap and vertical
// margins on the main axis and horizontal margins on the cross axis.
TEST_F(DecoupledListOrientationHelperTest, VerticalItemGeometry) {
  auto helper = CreateOrientationHelper(Orientation::kVertical);
  auto item_holder = CreateItemHolder(Orientation::kVertical);

  EXPECT_FLOAT_EQ(helper->GetDecoratedMeasurement(item_holder.get()), 113.f);
  EXPECT_FLOAT_EQ(helper->GetDecoratedMeasurementInOther(item_holder.get()),
                  144.f);
  EXPECT_FLOAT_EQ(helper->GetDecoratedStart(item_holder.get()), 181.f);
  EXPECT_FLOAT_EQ(helper->GetDecoratedEnd(item_holder.get()), 294.f);
  EXPECT_FLOAT_EQ(helper->GetStart(item_holder.get()), 188.f);
  EXPECT_FLOAT_EQ(helper->GetEnd(item_holder.get()), 280.f);
  EXPECT_FLOAT_EQ(helper->GetItemHolderMainMargin(item_holder.get()), 12.f);
  EXPECT_FLOAT_EQ(helper->GetItemHolderCrossMargin(item_holder.get()), 11.f);
}

// Verifies that horizontal item geometry includes the main-axis gap and
// horizontal margins on the main axis and vertical margins on the cross axis.
TEST_F(DecoupledListOrientationHelperTest, HorizontalItemGeometry) {
  auto helper = CreateOrientationHelper(Orientation::kHorizontal);
  auto item_holder = CreateItemHolder(Orientation::kHorizontal);

  EXPECT_FLOAT_EQ(helper->GetDecoratedMeasurement(item_holder.get()), 151.f);
  EXPECT_FLOAT_EQ(helper->GetDecoratedMeasurementInOther(item_holder.get()),
                  106.f);
  EXPECT_FLOAT_EQ(helper->GetDecoratedStart(item_holder.get()), 82.f);
  EXPECT_FLOAT_EQ(helper->GetDecoratedEnd(item_holder.get()), 233.f);
  EXPECT_FLOAT_EQ(helper->GetStart(item_holder.get()), 89.f);
  EXPECT_FLOAT_EQ(helper->GetEnd(item_holder.get()), 220.f);
  EXPECT_FLOAT_EQ(helper->GetItemHolderMainMargin(item_holder.get()), 11.f);
  EXPECT_FLOAT_EQ(helper->GetItemHolderCrossMargin(item_holder.get()), 12.f);
}

// Verifies that all item geometry queries safely return zero for a null item in
// both vertical and horizontal helpers.
TEST_F(DecoupledListOrientationHelperTest, NullItemGeometry) {
  for (Orientation orientation :
       {Orientation::kVertical, Orientation::kHorizontal}) {
    auto helper = CreateOrientationHelper(orientation);

    EXPECT_FLOAT_EQ(helper->GetDecoratedMeasurement(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetDecoratedMeasurementInOther(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetDecoratedStart(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetDecoratedEnd(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetStart(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetEnd(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetItemHolderMainMargin(nullptr), 0.f);
    EXPECT_FLOAT_EQ(helper->GetItemHolderCrossMargin(nullptr), 0.f);
  }
}

}  // namespace list
}  // namespace lynx

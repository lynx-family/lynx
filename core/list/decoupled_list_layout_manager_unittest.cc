
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_layout_manager.h"

#include "core/list/decoupled_list_container_impl.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "testing/fiber_data_source.h"
#include "testing/mock_list_element.h"
#include "testing/mock_list_item_element.h"
#include "testing/radon_data_source.h"
#include "testing/utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {

using ::testing::_;
using ::testing::Return;

class ListLayoutManagerTest : public ::testing::Test {
 public:
  ListLayoutManagerTest() = default;
  ~ListLayoutManagerTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_adapter_ = list_container_impl_->list_adapter();
    list_layout_manager_ = list_container_impl_->list_layout_manager();
  }

  void InitFiberDataSource() {
    testing::InsertAction insert_action{
        .insert_ops_ = {
            {.position_ = 0, "A_0", 100, false, false, false, false},
            {.position_ = 1, "B_1", 100, false, false, false, false},
            {.position_ = 2, "C_2", 100, false, false, false, false},
            {.position_ = 3, "D_3", 100, false, false, false, false},
            {.position_ = 4, "E_4", 100, false, false, false, false},
            {.position_ = 5, "F_5", 100, false, false, false, false},
            {.position_ = 6, "G_6", 100, false, false, false, false},
            {.position_ = 7, "H_7", 100, false, false, false, false},
            {.position_ = 8, "I_8", 100, false, false, false, false},
            {.position_ = 9, "J_9", 100, false, false, false, false},
        }};
    testing::FiberDataSource fiber_data_source{
        .insert_action_ = insert_action,
    };
    LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
        FiberUpdateListInfo, lepus::Value(fiber_data_source.Resolve()));
    list_container_impl_->ResolveAttribute(*key, value);
    list_container_impl_->PropsUpdateFinish();
  }

  void InitLayoutAttrs(std::string list_type, int span_count,
                       std::string scroll_orientation, float main_axis_gap,
                       float cross_axis_gap, float list_main_size,
                       float list_cross_size) {
    {
      LIST_CONTAINER_DEFINE_PROP_VALUE(ListType, String, list_type);
      list_container_impl_->ResolveAttribute(*key, *value);
      list_layout_manager_ = list_container_impl_->list_layout_manager();
    }
    {
      LIST_CONTAINER_DEFINE_PROP_VALUE(SpanCount, Number, span_count);
      list_container_impl_->ResolveAttribute(*key, *value);
    }
    {
      LIST_CONTAINER_DEFINE_PROP_VALUE(ScrollOrientation, String,
                                       scroll_orientation);
      list_container_impl_->ResolveAttribute(*key, *value);
    }
    list_container_impl_->ResolveListAxisGap(
        tasm::CSSPropertyID::kPropertyIDListMainAxisGap, main_axis_gap);
    list_container_impl_->ResolveListAxisGap(
        tasm::CSSPropertyID::kPropertyIDListCrossAxisGap, cross_axis_gap);
    if (scroll_orientation == kPropValueScrollOrientationVertical) {
      mock_list_element_->height_ = list_main_size;
      mock_list_element_->width_ = list_cross_size;
    } else {
      mock_list_element_->height_ = list_cross_size;
      mock_list_element_->width_ = list_main_size;
    }
    list_container_impl_->PropsUpdateFinish();
  }

  ListOrientationHelper* GetOrientationHelper() {
    return list_layout_manager_->list_orientation_helper_.get();
  }
};

TEST_F(ListLayoutManagerTest, InitLayoutAttrs) {
  InitLayoutAttrs("waterfall", 3, "horizontal", 10.f, 5.f, 2000.f, 1000.f);
  EXPECT_TRUE(list_layout_manager_->CanScrollHorizontally());
  EXPECT_EQ(list_layout_manager_->span_count(), 3);
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager_->main_axis_gap(), 10.f));
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager_->cross_axis_gap(), 5.f));
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager_->content_offset(), 0.f));
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager_->content_size(), 0.f));
  EXPECT_TRUE(
      base::FloatsEqual(GetOrientationHelper()->GetMeasurement(), 2000.f));
  EXPECT_TRUE(base::FloatsEqual(GetOrientationHelper()->GetMeasurementInOther(),
                                1000.f));
}

}  // namespace list
}  // namespace lynx

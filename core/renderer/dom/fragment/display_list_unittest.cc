// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

#include <memory>
#include <utility>
#include <vector>

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class DisplayListTest : public ::testing::Test {
 protected:
  void SetUp() override { display_list_ = std::make_unique<DisplayList>(); }

  void TearDown() override { display_list_.reset(); }

  std::unique_ptr<DisplayList> display_list_;
};

TEST_F(DisplayListTest, EmptyDisplayList) {
  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 0u);
  EXPECT_EQ(display_list_->GetContentIntDataSize(), 0u);
  EXPECT_EQ(display_list_->GetContentFloatDataSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertyIntDataSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertyFloatDataSize(), 0u);
}

TEST_F(DisplayListTest, AddSingleOperation) {
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));

  // single operation, content_int_data_.size() == 3, content_float_data_.size()
  // == 0
  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 1u);

  // [int_count, float_count, int_data...]
  // [1, 0, 0xFF0000FF]
  EXPECT_EQ(display_list_->GetContentIntDataSize(), 3u);
  // No float parameters, so content_float_data_ should be empty
  EXPECT_EQ(display_list_->GetContentFloatDataSize(), 0u);

  const int32_t* op_types_data = display_list_->GetContentOpTypesData();
  const int32_t* int_data_data = display_list_->GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kFill));
  // First int data is int_count (1), second is float_count (0), followed by
  // actual data
  EXPECT_EQ(int_data_data[0], 1);  // int_count
  EXPECT_EQ(int_data_data[1], 0);  // float_count
  EXPECT_EQ(int_data_data[2],
            static_cast<int32_t>(0xFF0000FF));  // actual data
}

TEST_F(DisplayListTest, AddOperationWithFloatData) {
  display_list_->AddOperation(DisplayListOpType::kDrawView, 123, 3.14f);

  const int32_t* op_types_data = display_list_->GetContentOpTypesData();
  const int32_t* int_data_data = display_list_->GetContentIntData();
  const float* float_data_data = display_list_->GetContentFloatData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);
  EXPECT_NE(float_data_data, nullptr);

  EXPECT_EQ(op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kDrawView));

  // Int data: [int_count, float_count, int_data...]
  EXPECT_EQ(int_data_data[0], 1);  // int_count
  EXPECT_EQ(int_data_data[1],
            1);                      // float_count (stored in int_data)
  EXPECT_EQ(int_data_data[2], 123);  // actual int data

  // Float data: [actual float data]
  EXPECT_EQ(display_list_->GetContentFloatDataSize(), 1u);  // 1 float parameter
  EXPECT_FLOAT_EQ(float_data_data[0], 3.14f);               // actual data
}

TEST_F(DisplayListTest, AddOperationWithMixedParams) {
  // Test AddOperation with both int and float parameters (equivalent to old
  // AddOperationWithData)
  display_list_->AddOperation(DisplayListOpType::kBegin, 1, 2, 3,
                              4,                        // int params
                              1.1f, 2.2f, 3.3f, 4.4f);  // float params

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 1u);
  EXPECT_EQ(display_list_->GetContentIntDataSize(), 6u);  // 2 counts + 4 params
  EXPECT_EQ(display_list_->GetContentFloatDataSize(), 4u);  // 4 float params

  const int32_t* op_types_data = display_list_->GetContentOpTypesData();
  const int32_t* int_data_data = display_list_->GetContentIntData();
  const float* float_data_data = display_list_->GetContentFloatData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);
  EXPECT_NE(float_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(int_data_data[0], 4);  // int_count
  EXPECT_EQ(int_data_data[1], 4);  // float_count

  // Check parameter data - int params start at index 2
  EXPECT_EQ(int_data_data[2], 1);
  EXPECT_EQ(int_data_data[3], 2);
  EXPECT_EQ(int_data_data[4], 3);
  EXPECT_EQ(int_data_data[5], 4);

  // Float params start at index 0
  EXPECT_FLOAT_EQ(float_data_data[0], 1.1f);
  EXPECT_FLOAT_EQ(float_data_data[1], 2.2f);
  EXPECT_FLOAT_EQ(float_data_data[2], 3.3f);
  EXPECT_FLOAT_EQ(float_data_data[3], 4.4f);
}

TEST_F(DisplayListTest, AddMultipleOperations) {
  display_list_->AddOperation(DisplayListOpType::kBegin, 0);
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));
  display_list_->AddOperation(DisplayListOpType::kEnd, 0);

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 3u);
  EXPECT_EQ(display_list_->GetContentIntDataSize(),
            9u);  // 3 elements per operation (int_count, float_count, param)
  EXPECT_EQ(display_list_->GetContentFloatDataSize(),
            0u);  // No float parameters

  const int32_t* op_types_data = display_list_->GetContentOpTypesData();
  EXPECT_NE(op_types_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(op_types_data[2], static_cast<int32_t>(DisplayListOpType::kEnd));
}

TEST_F(DisplayListTest, ClearDisplayList) {
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));
  display_list_->AddOperation(DisplayListOpType::kDrawView, 123);

  display_list_->Clear();

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 0u);
  EXPECT_EQ(display_list_->GetContentIntDataSize(), 0u);
  EXPECT_EQ(display_list_->GetContentFloatDataSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertyIntDataSize(), 0u);
  EXPECT_EQ(display_list_->GetSubtreePropertyFloatDataSize(), 0u);
}

TEST_F(DisplayListTest, DirectArrayAccess) {
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));
  display_list_->AddOperation(DisplayListOpType::kDrawView, 123);

  const int32_t* op_types_data = display_list_->GetContentOpTypesData();
  const int32_t* int_data_data = display_list_->GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kDrawView));

  // Int data layout: [int_count1, float_count1, param1, int_count2,
  // float_count2, param2]
  EXPECT_EQ(int_data_data[0], 1);  // int_count for first operation
  EXPECT_EQ(int_data_data[1], 0);  // float_count for first operation
  EXPECT_EQ(int_data_data[2], static_cast<int32_t>(0xFF0000FF));  // first param
  EXPECT_EQ(int_data_data[3], 1);    // int_count for second operation
  EXPECT_EQ(int_data_data[4], 0);    // float_count for second operation
  EXPECT_EQ(int_data_data[5], 123);  // second param

  // No float parameters, so content_float_data_ should be empty
  EXPECT_EQ(display_list_->GetContentFloatDataSize(), 0u);
}

TEST_F(DisplayListTest, MoveSemantics) {
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));
  display_list_->AddOperation(DisplayListOpType::kDrawView, 123);

  DisplayList moved_list = std::move(*display_list_);

  const int32_t* moved_op_types_data = moved_list.GetContentOpTypesData();
  EXPECT_NE(moved_op_types_data, nullptr);
  EXPECT_EQ(moved_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(moved_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kDrawView));
}

TEST_F(DisplayListTest, LargeDataOperations) {
  const size_t kLargeSize = 1000;

  for (size_t i = 0; i < kLargeSize; ++i) {
    display_list_->AddOperation(DisplayListOpType::kFill,
                                static_cast<int32_t>(i));
  }

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), kLargeSize);
  EXPECT_EQ(display_list_->GetContentIntDataSize(),
            kLargeSize *
                3);  // 3 elements per operation (int_count, float_count, param)
  EXPECT_EQ(display_list_->GetContentFloatDataSize(),
            0u);  // No float parameters

  // Verify some data points
  const int32_t* op_types_data = display_list_->GetContentOpTypesData();
  const int32_t* int_data_data = display_list_->GetContentIntData();
  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(int_data_data[0], 1);  // int_count for first operation
  EXPECT_EQ(int_data_data[1], 0);  // float_count for first operation
  EXPECT_EQ(int_data_data[2], 0);  // first parameter
  EXPECT_EQ(int_data_data[(kLargeSize - 1) * 3],
            1);  // int_count for last operation
  EXPECT_EQ(int_data_data[(kLargeSize - 1) * 3 + 1],
            0);  // float_count for last operation
  EXPECT_EQ(int_data_data[(kLargeSize - 1) * 3 + 2],
            static_cast<int32_t>(kLargeSize - 1));  // last parameter
}

TEST_F(DisplayListTest, MixedOperationTypes) {
  display_list_->AddOperation(DisplayListOpType::kBegin, 0);
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kTransform,
                              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kClip, 1, 0.0f);
  display_list_->AddOperation(DisplayListOpType::kText, 2, 0.0f);
  display_list_->AddOperation(DisplayListOpType::kImage, 3, 0.0f);
  display_list_->AddOperation(DisplayListOpType::kCustom, 4, 0.0f);
  display_list_->AddOperation(DisplayListOpType::kEnd, 5);

  // Content operations: kBegin, kText, kImage, kCustom, kEnd (5 operations)
  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 5u);
  // Subtree property operations: kTransform, kClip (2 operations)
  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 2u);

  // Verify content operations are correctly stored
  const int32_t* content_op_types_data = display_list_->GetContentOpTypesData();
  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(content_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(content_op_types_data[2],
            static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(content_op_types_data[3],
            static_cast<int32_t>(DisplayListOpType::kCustom));
  EXPECT_EQ(content_op_types_data[4],
            static_cast<int32_t>(DisplayListOpType::kEnd));

  // Verify subtree property operations are correctly stored
  const int32_t* subtree_op_types_data =
      display_list_->GetSubtreePropertyOpTypesData();
  EXPECT_NE(subtree_op_types_data, nullptr);
  EXPECT_EQ(subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));
  EXPECT_EQ(subtree_op_types_data[1],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kClip));
}

TEST_F(DisplayListTest, SubtreePropertySeparation) {
  // Add content operations
  display_list_->AddOperation(DisplayListOpType::kBegin, 0);
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));
  display_list_->AddOperation(DisplayListOpType::kEnd, 0);

  // Add subtree property operations
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kTransform,
                              1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
                              9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,
                              16.0f);
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kClip,
                              static_cast<int32_t>(4), 5.0f, 6.0f, 7.0f, 8.0f);

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 3u);
  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 2u);

  // Verify content operations
  const int32_t* content_op_types_data = display_list_->GetContentOpTypesData();
  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(content_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(content_op_types_data[2],
            static_cast<int32_t>(DisplayListOpType::kEnd));

  // Verify subtree property operations
  const int32_t* subtree_op_types_data =
      display_list_->GetSubtreePropertyOpTypesData();
  EXPECT_NE(subtree_op_types_data, nullptr);
  EXPECT_EQ(subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));
  EXPECT_EQ(subtree_op_types_data[1],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kClip));

  // Test fast update of subtree properties by clearing and re-adding
  display_list_->ClearSubtreeProperties();
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kTransform,
                              1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
                              9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,
                              16.0f);

  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 1u);
  const int32_t* new_subtree_op_types_data =
      display_list_->GetSubtreePropertyOpTypesData();
  EXPECT_NE(new_subtree_op_types_data, nullptr);
  EXPECT_EQ(new_subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));
  EXPECT_EQ(display_list_->GetSubtreePropertyIntDataSize(), 2u);
  EXPECT_EQ(display_list_->GetSubtreePropertyFloatDataSize(), 16u);

  // Content operations should remain unchanged
  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 3u);
}

TEST_F(DisplayListTest, ClearSubtreeProperties) {
  // Add mixed operations
  display_list_->AddOperation(DisplayListOpType::kBegin, 0);
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kTransform,
                              static_cast<int32_t>(1), 2.0f);
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kClip,
                              static_cast<int32_t>(3), 4.0f);
  display_list_->AddOperation(DisplayListOpType::kEnd, 0);

  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 2u);

  // Clear only subtree properties
  display_list_->ClearSubtreeProperties();

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 2u);
  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 0u);

  // Verify content operations are preserved
  const int32_t* content_op_types_data = display_list_->GetContentOpTypesData();
  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(content_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kEnd));
}

TEST_F(DisplayListTest, TemplateCategorySelection) {
  // Test explicit template category specification
  display_list_->AddOperation(DisplayListOpType::kFill,
                              static_cast<int32_t>(0xFF0000FF));
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kTransform,
                              1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
                              9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,
                              16.0f);
  display_list_->AddOperation(DisplayListOpType::kDrawView, 123);
  display_list_->AddOperation(DisplayListSubtreePropertyOpType::kClip, 10.0f,
                              20.0f, 30.0f, 40.0f);

  EXPECT_EQ(display_list_->GetContentOpTypesSize(), 2u);
  EXPECT_EQ(display_list_->GetSubtreePropertyOpTypesSize(), 2u);

  // Verify content operations
  const int32_t* content_op_types_data = display_list_->GetContentOpTypesData();
  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(content_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kDrawView));

  // Verify subtree property operations
  const int32_t* subtree_op_types_data =
      display_list_->GetSubtreePropertyOpTypesData();
  EXPECT_NE(subtree_op_types_data, nullptr);
  EXPECT_EQ(subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));
  EXPECT_EQ(subtree_op_types_data[1],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kClip));

  // Verify data integrity
  const int32_t* content_int_data = display_list_->GetContentIntData();
  const int32_t* subtree_int_data = display_list_->GetSubtreePropertyIntData();
  EXPECT_NE(content_int_data, nullptr);
  EXPECT_NE(subtree_int_data, nullptr);

  EXPECT_EQ(content_int_data[0], 1);  // int_count for Fill
  EXPECT_EQ(content_int_data[1], 0);  // float_count for Fill
  EXPECT_EQ(content_int_data[2],
            static_cast<int32_t>(0xFF0000FF));  // color param

  EXPECT_EQ(content_int_data[3], 1);    // int_count for DrawView
  EXPECT_EQ(content_int_data[4], 0);    // float_count for DrawView
  EXPECT_EQ(content_int_data[5], 123);  // view_id param

  // Transform has 16 float parameters, no int parameters
  EXPECT_EQ(subtree_int_data[0], 0);   // int_count for Transform
  EXPECT_EQ(subtree_int_data[1], 16);  // float_count for Transform

  // Clip has 4 float parameters, no int parameters
  EXPECT_EQ(subtree_int_data[2], 0);  // int_count for Clip
  EXPECT_EQ(subtree_int_data[3], 4);  // float_count for Clip
}

TEST_F(DisplayListTest, TestMarkRootNeedClipBounds) {
  EXPECT_FALSE(display_list_->RootNeedClipBounds());

  display_list_->MarkRootNeedClipBounds();

  EXPECT_TRUE(display_list_->RootNeedClipBounds());
}

}  // namespace tasm
}  // namespace lynx

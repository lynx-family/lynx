// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <vector>

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class DisplayListBuilderTest : public ::testing::Test {
 protected:
  void SetUp() override { builder_ = std::make_unique<DisplayListBuilder>(); }

  void TearDown() override { builder_.reset(); }

  std::unique_ptr<DisplayListBuilder> builder_;
};

TEST_F(DisplayListBuilderTest, EmptyBuilder) {}

TEST_F(DisplayListBuilderTest, BeginOperation) {
  auto& result = builder_->Begin(10.0f, 20.0f, 100.0f, 200.0f);

  EXPECT_EQ(&result, builder_.get());  // Method chaining

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();
  const float* float_data_data = display_list.GetContentFloatData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);
  EXPECT_NE(float_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kBegin));

  // Check parameter data structure - optimized to only store float params
  EXPECT_EQ(int_data_data[0], 0);  // int_count (no int params)
  EXPECT_EQ(int_data_data[1], 4);  // float_count (4 float params)

  // Check float parameters directly
  EXPECT_FLOAT_EQ(float_data_data[0], 10.0f);
  EXPECT_FLOAT_EQ(float_data_data[1], 20.0f);
  EXPECT_FLOAT_EQ(float_data_data[2], 100.0f);
  EXPECT_FLOAT_EQ(float_data_data[3], 200.0f);
}

TEST_F(DisplayListBuilderTest, EndOperation) {
  builder_->End();

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kEnd));
  // With optimized AddOperation, End() has no parameters but still stores
  // counts [0, 0]
  EXPECT_EQ(display_list.GetContentIntDataSize(),
            2u);  // [int_count=0, float_count=0]
  EXPECT_EQ(display_list.GetContentFloatDataSize(), 0u);
  EXPECT_EQ(int_data_data[0], 0);  // int_count
  EXPECT_EQ(int_data_data[1], 0);  // float_count
}

TEST_F(DisplayListBuilderTest, FillOperation) {
  uint32_t color = 0xFF00FF00;
  builder_->Fill(color);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kFill));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 1);  // int_count
  EXPECT_EQ(int_data_data[1], 0);  // float_count
  EXPECT_EQ(int_data_data[2],
            static_cast<int32_t>(color));                 // actual param
  EXPECT_EQ(display_list.GetContentFloatDataSize(), 0u);  // No float parameters
}

TEST_F(DisplayListBuilderTest, DrawViewOperation) {
  int view_id = 42;
  builder_->DrawView(view_id);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kDrawView));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 1);                         // int_count
  EXPECT_EQ(int_data_data[1], 0);                         // float_count
  EXPECT_EQ(int_data_data[2], view_id);                   // actual param
  EXPECT_EQ(display_list.GetContentFloatDataSize(), 0u);  // No float parameters
}

TEST_F(DisplayListBuilderTest, TransformOperation) {
  float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f, e = 50.0f, f = 100.0f;
  builder_->Transform(a, b, c, d, e, f);

  DisplayList display_list = builder_->Build();

  const int32_t* subtree_op_types_data =
      display_list.GetSubtreePropertyOpTypesData();
  const int32_t* subtree_int_data = display_list.GetSubtreePropertyIntData();
  const float* subtree_float_data = display_list.GetSubtreePropertyFloatData();

  EXPECT_NE(subtree_op_types_data, nullptr);
  EXPECT_NE(subtree_int_data, nullptr);
  EXPECT_NE(subtree_float_data, nullptr);

  EXPECT_EQ(subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));

  // Check parameter structure: [int_count, float_count]
  EXPECT_EQ(subtree_int_data[0], 0);  // int_count
  EXPECT_EQ(subtree_int_data[1], 6);  // float_count

  // Check transform matrix values (float params start at index 0)
  EXPECT_EQ(display_list.GetSubtreePropertyFloatDataSize(), 6u);
  EXPECT_FLOAT_EQ(subtree_float_data[0], a);
  EXPECT_FLOAT_EQ(subtree_float_data[1], b);
  EXPECT_FLOAT_EQ(subtree_float_data[2], c);
  EXPECT_FLOAT_EQ(subtree_float_data[3], d);
  EXPECT_FLOAT_EQ(subtree_float_data[4], e);
  EXPECT_FLOAT_EQ(subtree_float_data[5], f);
}

TEST_F(DisplayListBuilderTest, ClipOperation) {
  float x = 10.0f, y = 20.0f, width = 100.0f, height = 200.0f;
  builder_->Clip(x, y, width, height);

  DisplayList display_list = builder_->Build();

  const int32_t* subtree_op_types_data =
      display_list.GetSubtreePropertyOpTypesData();
  const int32_t* subtree_int_data = display_list.GetSubtreePropertyIntData();
  const float* subtree_float_data = display_list.GetSubtreePropertyFloatData();

  EXPECT_NE(subtree_op_types_data, nullptr);
  EXPECT_NE(subtree_int_data, nullptr);
  EXPECT_NE(subtree_float_data, nullptr);

  EXPECT_EQ(subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kClip));

  // Check parameter structure - optimized to only store float params
  EXPECT_EQ(subtree_int_data[0], 0);  // int_count (no int params)
  EXPECT_EQ(subtree_int_data[1], 4);  // float_count (4 float params)

  // Check float parameters directly
  EXPECT_EQ(display_list.GetSubtreePropertyFloatDataSize(), 4u);
  EXPECT_FLOAT_EQ(subtree_float_data[0], x);
  EXPECT_FLOAT_EQ(subtree_float_data[1], y);
  EXPECT_FLOAT_EQ(subtree_float_data[2], width);
  EXPECT_FLOAT_EQ(subtree_float_data[3], height);
}

TEST_F(DisplayListBuilderTest, MethodChaining) {
  builder_->Begin(0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF0000FF)
      .DrawView(123)
      .Transform(1.0f, 0.0f, 0.0f, 1.0f, 50.0f, 50.0f)
      .Clip(10.0f, 10.0f, 80.0f, 80.0f)
      .End();

  DisplayList display_list = builder_->Build();

  // Verify content operations (Begin, Fill, DrawView, End)
  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* subtree_op_types_data =
      display_list.GetSubtreePropertyOpTypesData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(subtree_op_types_data, nullptr);

  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(content_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(content_op_types_data[2],
            static_cast<int32_t>(DisplayListOpType::kDrawView));
  EXPECT_EQ(content_op_types_data[3],
            static_cast<int32_t>(DisplayListOpType::kEnd));

  // Verify subtree property operations (Transform, Clip)
  EXPECT_EQ(subtree_op_types_data[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));
  EXPECT_EQ(subtree_op_types_data[1],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kClip));
}

TEST_F(DisplayListBuilderTest, ClearBuilder) {
  builder_->Begin(0.0f, 0.0f, 100.0f, 100.0f).Fill(0xFF0000FF).End();

  builder_->Clear();

  DisplayList display_list = builder_->Build();
}

TEST_F(DisplayListBuilderTest, BuildMultipleTimes) {
  builder_->Begin(0.0f, 0.0f, 100.0f, 100.0f).Fill(0xFF0000FF);

  DisplayList display_list1 = builder_->Build();

  // Builder should be cleared after Build()

  // Add new operations
  builder_->DrawView(123).End();

  DisplayList display_list2 = builder_->Build();

  const int32_t* op_types_data = display_list2.GetContentOpTypesData();
  EXPECT_NE(op_types_data, nullptr);
  EXPECT_EQ(op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kDrawView));
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kEnd));
}

TEST_F(DisplayListBuilderTest, LargeOperationSequence) {
  const size_t kNumOperations = 100;

  builder_->Begin(0.0f, 0.0f, 100.0f, 100.0f);

  for (size_t i = 0; i < kNumOperations; ++i) {
    builder_->Fill(static_cast<uint32_t>(i));
  }

  builder_->End();

  DisplayList display_list = builder_->Build();

  // Verify first and last operations
  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  EXPECT_NE(op_types_data, nullptr);
  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(op_types_data[display_list.GetContentOpTypesSize() - 1],
            static_cast<int32_t>(DisplayListOpType::kEnd));

  // Find the first Fill operation data
  // Begin operation: 2 elements [int_count=0, float_count=4] + 4 float params
  // Each Fill operation: 3 elements [int_count=1, float_count=0, 1 int param]
  const int32_t* int_data_data = display_list.GetContentIntData();
  EXPECT_NE(int_data_data, nullptr);
  size_t current_data_index = 2;  // After Begin operation counts

  // Verify first fill operation
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(int_data_data[current_data_index], 1);      // int_count
  EXPECT_EQ(int_data_data[current_data_index + 1], 0);  // float_count
  EXPECT_EQ(int_data_data[current_data_index + 2],
            0);  // First fill color (param)

  // Find the 50th fill operation (index 50 in OpTypes, but 49th Fill)
  current_data_index += 3;  // Skip first Fill
  for (size_t i = 2; i < 50; ++i) {
    current_data_index += 3;  // Skip each Fill operation data
  }

  EXPECT_EQ(op_types_data[50], static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(int_data_data[current_data_index], 1);      // int_count
  EXPECT_EQ(int_data_data[current_data_index + 1], 0);  // float_count
  EXPECT_EQ(int_data_data[current_data_index + 2],
            49);  // 50th fill color (index 49)
}

TEST_F(DisplayListBuilderTest, ZeroValues) {
  builder_->Begin(0.0f, 0.0f, 0.0f, 0.0f)
      .Fill(0)
      .DrawView(0)
      .Transform(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f)
      .Clip(0.0f, 0.0f, 0.0f, 0.0f)
      .End();

  DisplayList display_list = builder_->Build();

  // Verify content operations
  EXPECT_EQ(display_list.GetContentOpTypesData()[0],
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(display_list.GetContentOpTypesData()[1],
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(display_list.GetContentOpTypesData()[2],
            static_cast<int32_t>(DisplayListOpType::kDrawView));
  EXPECT_EQ(display_list.GetContentOpTypesData()[3],
            static_cast<int32_t>(DisplayListOpType::kEnd));

  // Verify subtree property operations
  EXPECT_EQ(display_list.GetSubtreePropertyOpTypesData()[0],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kTransform));
  EXPECT_EQ(display_list.GetSubtreePropertyOpTypesData()[1],
            static_cast<int32_t>(DisplayListSubtreePropertyOpType::kClip));

  // Check specific zero values
  // Begin operation: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetContentIntData()[0], 0);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[1], 4);  // float_count
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[0], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[1], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[2], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[3], 0.0f);

  // Fill operation: [int_count=1, float_count=0, 1 int param]
  EXPECT_EQ(display_list.GetContentIntData()[2], 1);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[3], 0);  // float_count
  EXPECT_EQ(display_list.GetContentIntData()[4], 0);  // Fill color param

  // DrawView operation: [int_count=1, float_count=0, 1 int param]
  EXPECT_EQ(display_list.GetContentIntData()[5], 1);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[6], 0);  // float_count
  EXPECT_EQ(display_list.GetContentIntData()[7], 0);  // DrawView param

  // Transform operation: [int_count=0, float_count=6, 6 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[0], 0);  // int_count
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[1], 6);  // float_count
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[0], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[1], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[2], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[3], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[4], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[5], 0.0f);

  // Clip operation: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[2], 0);  // int_count
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[3], 4);  // float_count
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[6], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[7], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[8], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[9], 0.0f);
}

TEST_F(DisplayListBuilderTest, NegativeValues) {
  builder_->Begin(-10.0f, -20.0f, -100.0f, -200.0f)
      .Transform(-1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f)
      .Clip(-5.0f, -15.0f, -50.0f, -100.0f);

  DisplayList display_list = builder_->Build();

  // Check Begin operation with negative values (content operation)
  // Begin: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetContentIntData()[0],
            0);  // int_count (no int params)
  EXPECT_EQ(display_list.GetContentIntData()[1], 4);  // float_count

  // Check float parameters directly for Begin
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[0], -10.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[1], -20.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[2], -100.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[3], -200.0f);

  // Check Transform operation with negative values (subtree property)
  // Transform: [int_count=0, float_count=6, 6 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[0], 0);  // int_count
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[1], 6);  // float_count

  // Check Transform float parameters directly
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[0], -1.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[1], -2.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[2], -3.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[3], -4.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[4], -5.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[5], -6.0f);

  // Check Clip operation with negative values (subtree property)
  // Clip: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[2],
            0);  // int_count for Clip
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[3],
            4);  // float_count for Clip

  // Check Clip float parameters directly
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[6], -5.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[7], -15.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[8], -50.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[9], -100.0f);
}

}  // namespace tasm
}  // namespace lynx

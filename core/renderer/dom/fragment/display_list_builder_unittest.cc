// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <vector>

#include "core/renderer/starlight/style/borders_data.h"
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

TEST_F(DisplayListBuilderTest, DrawImageOperation) {
  int image_id = 123;
  builder_->DrawImage(image_id);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 1);                         // int_count
  EXPECT_EQ(int_data_data[1], 0);                         // float_count
  EXPECT_EQ(int_data_data[2], image_id);                  // actual param
  EXPECT_EQ(display_list.GetContentFloatDataSize(), 0u);  // No float parameters
}

TEST_F(DisplayListBuilderTest, DrawTextOperation) {
  int text_id = 456;
  builder_->DrawText(text_id);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kText));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 1);                         // int_count
  EXPECT_EQ(int_data_data[1], 0);                         // float_count
  EXPECT_EQ(int_data_data[2], text_id);                   // actual param
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
      .DrawImage(456)
      .DrawText(789)
      .Transform(1.0f, 0.0f, 0.0f, 1.0f, 50.0f, 50.0f)
      .Clip(10.0f, 10.0f, 80.0f, 80.0f)
      .End();

  DisplayList display_list = builder_->Build();

  // Verify content operations (Begin, Fill, DrawView, DrawImage, DrawText, End)
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
            static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(content_op_types_data[4],
            static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(content_op_types_data[5],
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
    if (i % 3 == 0) {
      builder_->DrawImage(static_cast<int>(i));
    }
    if (i % 5 == 0) {
      builder_->DrawText(static_cast<int>(i * 2));
    }
  }

  builder_->End();

  DisplayList display_list = builder_->Build();

  // Verify first and last operations
  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  EXPECT_NE(op_types_data, nullptr);
  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(op_types_data[display_list.GetContentOpTypesSize() - 1],
            static_cast<int32_t>(DisplayListOpType::kEnd));

  // Verify some DrawImage and DrawText operations exist
  bool found_draw_image = false;
  bool found_draw_text = false;
  int draw_image_count = 0;
  int draw_text_count = 0;

  for (size_t i = 0; i < display_list.GetContentOpTypesSize(); ++i) {
    if (op_types_data[i] == static_cast<int32_t>(DisplayListOpType::kImage)) {
      found_draw_image = true;
      draw_image_count++;
    }
    if (op_types_data[i] == static_cast<int32_t>(DisplayListOpType::kText)) {
      found_draw_text = true;
      draw_text_count++;
    }
  }

  EXPECT_TRUE(found_draw_image);
  EXPECT_TRUE(found_draw_text);

  // Verify expected counts (i % 3 == 0 and i % 5 == 0 patterns)
  // DrawImage should appear for i = 0, 3, 6, 9, ..., 99 (34 times)
  // DrawText should appear for i = 0, 5, 10, 15, ..., 95 (20 times)
  EXPECT_EQ(draw_image_count, 34);
  EXPECT_EQ(draw_text_count, 20);

  // Verify first DrawImage and DrawText operations specifically
  bool found_first_draw_image = false;
  bool found_first_draw_text = false;
  const int32_t* int_data_data = display_list.GetContentIntData();

  // Find first DrawImage (should be at i=0, so image_id=0)
  for (size_t i = 0; i < display_list.GetContentOpTypesSize(); ++i) {
    if (op_types_data[i] == static_cast<int32_t>(DisplayListOpType::kImage)) {
      // Find corresponding data - need to calculate data index based on
      // position
      size_t data_index = 2;            // Skip Begin operation data
      for (size_t j = 1; j < i; ++j) {  // Skip Begin operation
        if (op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kFill) ||
            op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kDrawView) ||
            op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kImage) ||
            op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kText)) {
          data_index += 3;  // Each content operation uses 3 data elements
        }
      }
      EXPECT_EQ(int_data_data[data_index + 2],
                0);  // First DrawImage should have image_id=0
      found_first_draw_image = true;
      break;
    }
  }
  EXPECT_TRUE(found_first_draw_image);

  // Find first DrawText (should be at i=0, so text_id=0)
  for (size_t i = 0; i < display_list.GetContentOpTypesSize(); ++i) {
    if (op_types_data[i] == static_cast<int32_t>(DisplayListOpType::kText)) {
      // Find corresponding data - need to calculate data index based on
      // position
      size_t data_index = 2;            // Skip Begin operation data
      for (size_t j = 1; j < i; ++j) {  // Skip Begin operation
        if (op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kFill) ||
            op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kDrawView) ||
            op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kImage) ||
            op_types_data[j] ==
                static_cast<int32_t>(DisplayListOpType::kText)) {
          data_index += 3;  // Each content operation uses 3 data elements
        }
      }
      EXPECT_EQ(int_data_data[data_index + 2],
                0);  // First DrawText should have text_id=0
      found_first_draw_text = true;
      break;
    }
  }
  EXPECT_TRUE(found_first_draw_text);
}

TEST_F(DisplayListBuilderTest, ZeroValues) {
  builder_->Begin(0.0f, 0.0f, 0.0f, 0.0f)
      .Fill(0)
      .DrawView(0)
      .DrawImage(0)
      .DrawText(0)
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
            static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(display_list.GetContentOpTypesData()[4],
            static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(display_list.GetContentOpTypesData()[5],
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

  // DrawImage operation: [int_count=1, float_count=0, 1 int param]
  EXPECT_EQ(display_list.GetContentIntData()[8], 1);   // int_count
  EXPECT_EQ(display_list.GetContentIntData()[9], 0);   // float_count
  EXPECT_EQ(display_list.GetContentIntData()[10], 0);  // DrawImage param

  // DrawText operation: [int_count=1, float_count=0, 1 int param]
  EXPECT_EQ(display_list.GetContentIntData()[11], 1);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[12], 0);  // float_count
  EXPECT_EQ(display_list.GetContentIntData()[13], 0);  // DrawText param

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

TEST_F(DisplayListBuilderTest, DrawImageAndTextWithZeroValues) {
  builder_->DrawImage(0);
  builder_->DrawText(0);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  // Check DrawImage operation
  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(int_data_data[0], 1);  // int_count
  EXPECT_EQ(int_data_data[1], 0);  // float_count
  EXPECT_EQ(int_data_data[2], 0);  // image_id param

  // Check DrawText operation
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(int_data_data[3], 1);  // int_count
  EXPECT_EQ(int_data_data[4], 0);  // float_count
  EXPECT_EQ(int_data_data[5], 0);  // text_id param
}

TEST_F(DisplayListBuilderTest, DrawImageAndTextWithNegativeValues) {
  builder_->DrawImage(-123);
  builder_->DrawText(-456);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  // Check DrawImage operation with negative value
  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(int_data_data[0], 1);     // int_count
  EXPECT_EQ(int_data_data[1], 0);     // float_count
  EXPECT_EQ(int_data_data[2], -123);  // image_id param

  // Check DrawText operation with negative value
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(int_data_data[3], 1);     // int_count
  EXPECT_EQ(int_data_data[4], 0);     // float_count
  EXPECT_EQ(int_data_data[5], -456);  // text_id param
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

TEST_F(DisplayListBuilderTest, BorderOperation) {
  // Create a BordersData object with test values
  starlight::BordersData border_data;
  border_data.width_top = 2.0f;
  border_data.width_right = 3.0f;
  border_data.width_bottom = 4.0f;
  border_data.width_left = 5.0f;

  border_data.color_top = 0xFF0000FF;     // Red
  border_data.color_right = 0xFF00FF00;   // Green
  border_data.color_bottom = 0xFFFF0000;  // Blue
  border_data.color_left = 0xFF000000;    // Black

  border_data.style_top = starlight::BorderStyleType::kSolid;
  border_data.style_right = starlight::BorderStyleType::kDashed;
  border_data.style_bottom = starlight::BorderStyleType::kDotted;
  border_data.style_left = starlight::BorderStyleType::kDouble;

  builder_->Border(border_data);

  DisplayList display_list = builder_->Build();

  // Verify that border operation was added to content operations
  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* content_int_data = display_list.GetContentIntData();
  const float* content_float_data = display_list.GetContentFloatData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(content_int_data, nullptr);
  EXPECT_NE(content_float_data, nullptr);

  // Check that border operation was added
  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBorder));

  // Verify data structure - border operation should have both int and float
  // parameters Based on the implementation: 4 float widths + 4 int colors + 4
  // int styles = 8 int, 4 float
  EXPECT_EQ(content_int_data[0], 8);  // int_count
  EXPECT_EQ(content_int_data[1], 4);  // float_count
}

TEST_F(DisplayListBuilderTest, BorderOperationWithZeroValues) {
  // Create a BordersData object with zero values
  starlight::BordersData border_data;
  border_data.width_top = 0.0f;
  border_data.width_right = 0.0f;
  border_data.width_bottom = 0.0f;
  border_data.width_left = 0.0f;

  border_data.color_top = 0;
  border_data.color_right = 0;
  border_data.color_bottom = 0;
  border_data.color_left = 0;

  border_data.style_top = starlight::BorderStyleType::kNone;
  border_data.style_right = starlight::BorderStyleType::kNone;
  border_data.style_bottom = starlight::BorderStyleType::kNone;
  border_data.style_left = starlight::BorderStyleType::kNone;

  builder_->Border(border_data);

  DisplayList display_list = builder_->Build();

  // Verify that border operation was added even with zero values
  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* content_int_data = display_list.GetContentIntData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(content_int_data, nullptr);

  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBorder));
}

TEST_F(DisplayListBuilderTest, BorderOperationInMethodChaining) {
  // Test border operation in method chaining
  starlight::BordersData border_data;
  border_data.width_top = 1.0f;
  border_data.width_right = 2.0f;
  border_data.width_bottom = 3.0f;
  border_data.width_left = 4.0f;
  border_data.color_top = 0xFF0000FF;
  border_data.style_top = starlight::BorderStyleType::kSolid;

  builder_->Begin(0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF00FF00)
      .Border(border_data)
      .DrawView(123)
      .End();

  DisplayList display_list = builder_->Build();

  // Verify content operations
  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();

  EXPECT_NE(content_op_types_data, nullptr);

  // Check content operations - border should be between Fill and DrawView
  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kBegin));
  EXPECT_EQ(content_op_types_data[1],
            static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_EQ(content_op_types_data[2],
            static_cast<int32_t>(DisplayListOpType::kBorder));
  EXPECT_EQ(content_op_types_data[3],
            static_cast<int32_t>(DisplayListOpType::kDrawView));
  EXPECT_EQ(content_op_types_data[4],
            static_cast<int32_t>(DisplayListOpType::kEnd));
}

}  // namespace tasm
}  // namespace lynx

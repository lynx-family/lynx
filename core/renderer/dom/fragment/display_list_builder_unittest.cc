// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <vector>

#include "core/renderer/starlight/style/borders_data.h"
#include "core/style/transform/matrix44.h"
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
  auto& result = builder_->Begin(0, 10.0f, 20.0f, 100.0f, 200.0f);

  EXPECT_EQ(&result, builder_.get());  // Method chaining

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();
  const float* float_data_data = display_list.GetContentFloatData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);
  EXPECT_NE(float_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kBegin));

  // Check parameter data structure
  EXPECT_EQ(int_data_data[0], 1);  // int_count (1 int param)
  EXPECT_EQ(int_data_data[1], 4);  // float_count (4 float params)

  // Check parameters
  EXPECT_EQ(int_data_data[2], 0);
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
  EXPECT_EQ(int_data_data[0], 2);  // int_count
  EXPECT_EQ(int_data_data[1], 0);  // float_count
  EXPECT_EQ(int_data_data[2],
            static_cast<int32_t>(color));                 // actual param
  EXPECT_EQ(int_data_data[3], -1);                        // clip_index
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
  builder_->DrawImage(image_id, -1);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 2);                         // int_count
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
  transforms::Matrix44 matrix;
  matrix.preTranslate(50.0f, 100.0f, 0.0f);
  builder_->Transform(matrix);

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
  EXPECT_EQ(subtree_int_data[0], 0);   // int_count
  EXPECT_EQ(subtree_int_data[1], 16);  // float_count

  // Check transform matrix values (float params start at index 0)
  EXPECT_EQ(display_list.GetSubtreePropertyFloatDataSize(), 16u);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_FLOAT_EQ(subtree_float_data[i * 4 + j], matrix.rc(j, i));
    }
  }
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
  builder_->Begin(0, 0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF0000FF)
      .DrawView(123)
      .DrawImage(456, -1)
      .DrawText(789)
      .Transform(transforms::Matrix44())
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
  builder_->Begin(0, 0.0f, 0.0f, 100.0f, 100.0f).Fill(0xFF0000FF).End();

  builder_->Clear();

  DisplayList display_list = builder_->Build();
}

TEST_F(DisplayListBuilderTest, BuildMultipleTimes) {
  builder_->Begin(0, 0.0f, 0.0f, 100.0f, 100.0f).Fill(0xFF0000FF);

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

  builder_->Begin(0, 0.0f, 0.0f, 100.0f, 100.0f);

  for (size_t i = 0; i < kNumOperations; ++i) {
    builder_->Fill(static_cast<uint32_t>(i));
    if (i % 3 == 0) {
      builder_->DrawImage(static_cast<int>(i), -1);
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
      size_t data_index = 0;
      for (size_t j = 0; j < i; ++j) {
        data_index += 2 + int_data_data[data_index];
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
      size_t data_index = 0;
      for (size_t j = 0; j < i; ++j) {
        data_index += 2 + int_data_data[data_index];
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
  builder_->Begin(0, 0.0f, 0.0f, 0.0f, 0.0f)
      .Fill(0)
      .DrawView(0)
      .DrawImage(0, -1)
      .DrawText(0)
      .Transform(
          transforms::Matrix44(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
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
  // Begin operation: [int_count=1, float_count=4, 1 int param, 4 float params]
  EXPECT_EQ(display_list.GetContentIntData()[0], 1);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[1], 4);  // float_count
  EXPECT_EQ(display_list.GetContentIntData()[2], 0);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[0], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[1], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[2], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[3], 0.0f);

  // Fill operation: [int_count=2, float_count=0, 2 int params]
  EXPECT_EQ(display_list.GetContentIntData()[3], 2);   // int_count
  EXPECT_EQ(display_list.GetContentIntData()[4], 0);   // float_count
  EXPECT_EQ(display_list.GetContentIntData()[5], 0);   // Fill color param
  EXPECT_EQ(display_list.GetContentIntData()[6], -1);  // clip_index

  // DrawView operation: [int_count=1, float_count=0, 1 int param]
  EXPECT_EQ(display_list.GetContentIntData()[7], 1);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[8], 0);  // float_count
  EXPECT_EQ(display_list.GetContentIntData()[9], 0);  // DrawView param

  // DrawImage operation: [int_count=2, float_count=0, 2 int params]
  EXPECT_EQ(display_list.GetContentIntData()[10], 2);   // int_count
  EXPECT_EQ(display_list.GetContentIntData()[11], 0);   // float_count
  EXPECT_EQ(display_list.GetContentIntData()[12], 0);   // DrawImage param
  EXPECT_EQ(display_list.GetContentIntData()[13], -1);  // box_index

  // DrawText operation: [int_count=1, float_count=0, 1 int param]
  EXPECT_EQ(display_list.GetContentIntData()[14], 1);  // int_count
  EXPECT_EQ(display_list.GetContentIntData()[15], 0);  // float_count
  EXPECT_EQ(display_list.GetContentIntData()[16], 0);  // DrawText param

  // Transform operation: [int_count=0, float_count=16, 16 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[0], 0);   // int_count
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[1], 16);  // float_count
  for (int i = 0; i < 16; ++i) {
    EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[i], 0.0f);
  }

  // Clip operation: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[2], 0);  // int_count
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[3], 4);  // float_count
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[16], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[17], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[18], 0.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[19], 0.0f);
}

TEST_F(DisplayListBuilderTest, DrawImageAndTextWithZeroValues) {
  builder_->DrawImage(0, -1);
  builder_->DrawText(0);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  // Check DrawImage operation
  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(int_data_data[0], 2);  // int_count
  EXPECT_EQ(int_data_data[1], 0);  // float_count
  EXPECT_EQ(int_data_data[2], 0);  // image_id param
  EXPECT_EQ(int_data_data[3], -1);

  // Check DrawText operation
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(int_data_data[4], 1);  // int_count
  EXPECT_EQ(int_data_data[5], 0);  // float_count
  EXPECT_EQ(int_data_data[6], 0);  // text_id param
}

TEST_F(DisplayListBuilderTest, DrawImageAndTextWithNegativeValues) {
  builder_->DrawImage(-123, 1);
  builder_->DrawText(-456);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  // Check DrawImage operation with negative value
  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(int_data_data[0], 2);     // int_count
  EXPECT_EQ(int_data_data[1], 0);     // float_count
  EXPECT_EQ(int_data_data[2], -123);  // image_id param
  EXPECT_EQ(int_data_data[3], 1);

  // Check DrawText operation with negative value
  EXPECT_EQ(op_types_data[1], static_cast<int32_t>(DisplayListOpType::kText));
  EXPECT_EQ(int_data_data[4], 1);     // int_count
  EXPECT_EQ(int_data_data[5], 0);     // float_count
  EXPECT_EQ(int_data_data[6], -456);  // text_id param
}

TEST_F(DisplayListBuilderTest, NegativeValues) {
  transforms::Matrix44 matrix(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12,
                              -13, -14, -15, -16);
  builder_->Begin(0, -10.0f, -20.0f, -100.0f, -200.0f)
      .Transform(matrix)
      .Clip(-5.0f, -15.0f, -50.0f, -100.0f);

  DisplayList display_list = builder_->Build();

  // Check Begin operation with negative values (content operation)
  // Begin: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetContentIntData()[0],
            1);  // int_count (1 int params)
  EXPECT_EQ(display_list.GetContentIntData()[1], 4);  // float_count

  // Check parameters directly for Begin
  EXPECT_EQ(display_list.GetContentIntData()[2], 0);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[0], -10.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[1], -20.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[2], -100.0f);
  EXPECT_FLOAT_EQ(display_list.GetContentFloatData()[3], -200.0f);

  // Check Transform operation with negative values (subtree property)
  // Transform: [int_count=0, float_count=16, 16 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[0], 0);   // int_count
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[1], 16);  // float_count

  // Check Transform float parameters directly
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[i * 4 + j],
                      matrix.rc(j, i));
    }
  }

  // Check Clip operation with negative values (subtree property)
  // Clip: [int_count=0, float_count=4, 4 float params]
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[2],
            0);  // int_count for Clip
  EXPECT_EQ(display_list.GetSubtreePropertyIntData()[3],
            4);  // float_count for Clip

  // Check Clip float parameters directly
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[16], -5.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[17], -15.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[18], -50.0f);
  EXPECT_FLOAT_EQ(display_list.GetSubtreePropertyFloatData()[19], -100.0f);
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

  builder_->Border(1, 2, border_data);

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
  EXPECT_EQ(content_int_data[0], 10);  // int_count
  EXPECT_EQ(content_int_data[1], 0);   // float_count
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

  builder_->Border(1, 2, border_data);

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

  builder_->Begin(0, 0.0f, 0.0f, 100.0f, 100.0f)
      .Fill(0xFF00FF00)
      .Border(1, 2, border_data)
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

TEST_F(DisplayListBuilderTest, ClipRectPlain) {
  RoundedRectangle rect;
  rect.SetX(10.0f);
  rect.SetY(20.0f);
  rect.SetWidth(100.0f);
  rect.SetHeight(50.0f);

  builder_->ClipRect(rect);

  DisplayList display_list = builder_->Build();

  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* content_int_data = display_list.GetContentIntData();
  const float* content_float_data = display_list.GetContentFloatData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(content_int_data, nullptr);
  EXPECT_NE(content_float_data, nullptr);

  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kClipRect));

  EXPECT_EQ(content_int_data[0], 0);
  EXPECT_EQ(content_int_data[1], 4);

  EXPECT_FLOAT_EQ(content_float_data[0], 10.0f);
  EXPECT_FLOAT_EQ(content_float_data[1], 20.0f);
  EXPECT_FLOAT_EQ(content_float_data[2], 100.0f);
  EXPECT_FLOAT_EQ(content_float_data[3], 50.0f);
}

TEST_F(DisplayListBuilderTest, ClipRectRoundedCorners) {
  RoundedRectangle rect;
  rect.SetX(3.0f);
  rect.SetY(4.0f);
  rect.SetWidth(80.0f);
  rect.SetHeight(40.0f);
  rect.SetRadiusXTopLeft(5.0f);
  rect.SetRadiusYTopLeft(6.0f);
  rect.SetRadiusXTopRight(7.0f);
  rect.SetRadiusYTopRight(8.0f);
  rect.SetRadiusXBottomRight(9.0f);
  rect.SetRadiusYBottomRight(10.0f);
  rect.SetRadiusXBottomLeft(11.0f);
  rect.SetRadiusYBottomLeft(12.0f);

  builder_->ClipRect(rect);

  DisplayList display_list = builder_->Build();

  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* content_int_data = display_list.GetContentIntData();
  const float* content_float_data = display_list.GetContentFloatData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(content_int_data, nullptr);
  EXPECT_NE(content_float_data, nullptr);

  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kClipRect));

  EXPECT_EQ(content_int_data[0], 0);
  EXPECT_EQ(content_int_data[1], 12);

  EXPECT_FLOAT_EQ(content_float_data[0], 3.0f);
  EXPECT_FLOAT_EQ(content_float_data[1], 4.0f);
  EXPECT_FLOAT_EQ(content_float_data[2], 80.0f);
  EXPECT_FLOAT_EQ(content_float_data[3], 40.0f);

  EXPECT_FLOAT_EQ(content_float_data[4], 5.0f);
  EXPECT_FLOAT_EQ(content_float_data[5], 6.0f);
  EXPECT_FLOAT_EQ(content_float_data[6], 7.0f);
  EXPECT_FLOAT_EQ(content_float_data[7], 8.0f);
  EXPECT_FLOAT_EQ(content_float_data[8], 9.0f);
  EXPECT_FLOAT_EQ(content_float_data[9], 10.0f);
  EXPECT_FLOAT_EQ(content_float_data[10], 11.0f);
  EXPECT_FLOAT_EQ(content_float_data[11], 12.0f);
}

TEST_F(DisplayListBuilderTest, RecordBoxModelPlain) {
  RoundedRectangle rect;
  rect.SetX(10.0f);
  rect.SetY(20.0f);
  rect.SetWidth(100.0f);
  rect.SetHeight(50.0f);

  int32_t index = -1;
  builder_->RecordBoxModel(rect, index);

  EXPECT_EQ(index, 0);

  DisplayList display_list = builder_->Build();

  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* content_int_data = display_list.GetContentIntData();
  const float* content_float_data = display_list.GetContentFloatData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(content_int_data, nullptr);
  EXPECT_NE(content_float_data, nullptr);

  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kRecordBox));

  EXPECT_EQ(content_int_data[0], 0);
  EXPECT_EQ(content_int_data[1], 4);

  EXPECT_FLOAT_EQ(content_float_data[0], 10.0f);
  EXPECT_FLOAT_EQ(content_float_data[1], 20.0f);
  EXPECT_FLOAT_EQ(content_float_data[2], 100.0f);
  EXPECT_FLOAT_EQ(content_float_data[3], 50.0f);
}

TEST_F(DisplayListBuilderTest, RecordBoxModelRoundedCorners) {
  RoundedRectangle rect;
  rect.SetX(3.0f);
  rect.SetY(4.0f);
  rect.SetWidth(80.0f);
  rect.SetHeight(40.0f);
  rect.SetRadiusXTopLeft(5.0f);
  rect.SetRadiusYTopLeft(6.0f);
  rect.SetRadiusXTopRight(7.0f);
  rect.SetRadiusYTopRight(8.0f);
  rect.SetRadiusXBottomRight(9.0f);
  rect.SetRadiusYBottomRight(10.0f);
  rect.SetRadiusXBottomLeft(11.0f);
  rect.SetRadiusYBottomLeft(12.0f);

  int32_t index = -1;
  builder_->RecordBoxModel(rect, index);

  EXPECT_EQ(index, 0);

  DisplayList display_list = builder_->Build();

  const int32_t* content_op_types_data = display_list.GetContentOpTypesData();
  const int32_t* content_int_data = display_list.GetContentIntData();
  const float* content_float_data = display_list.GetContentFloatData();

  EXPECT_NE(content_op_types_data, nullptr);
  EXPECT_NE(content_int_data, nullptr);
  EXPECT_NE(content_float_data, nullptr);

  EXPECT_EQ(content_op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kRecordBox));

  EXPECT_EQ(content_int_data[0], 0);
  EXPECT_EQ(content_int_data[1], 12);

  EXPECT_FLOAT_EQ(content_float_data[0], 3.0f);
  EXPECT_FLOAT_EQ(content_float_data[1], 4.0f);
  EXPECT_FLOAT_EQ(content_float_data[2], 80.0f);
  EXPECT_FLOAT_EQ(content_float_data[3], 40.0f);

  EXPECT_FLOAT_EQ(content_float_data[4], 5.0f);
  EXPECT_FLOAT_EQ(content_float_data[5], 6.0f);
  EXPECT_FLOAT_EQ(content_float_data[6], 7.0f);
  EXPECT_FLOAT_EQ(content_float_data[7], 8.0f);
  EXPECT_FLOAT_EQ(content_float_data[8], 9.0f);
  EXPECT_FLOAT_EQ(content_float_data[9], 10.0f);
  EXPECT_FLOAT_EQ(content_float_data[10], 11.0f);
  EXPECT_FLOAT_EQ(content_float_data[11], 12.0f);
}

TEST_F(DisplayListBuilderTest, DrawImageWithBoxIndex) {
  int image_id = 123;
  int box_index = 5;
  builder_->DrawImage(image_id, box_index);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0], static_cast<int32_t>(DisplayListOpType::kImage));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 2);                         // int_count
  EXPECT_EQ(int_data_data[1], 0);                         // float_count
  EXPECT_EQ(int_data_data[2], image_id);                  // actual param
  EXPECT_EQ(int_data_data[3], box_index);                 // box_index
  EXPECT_EQ(display_list.GetContentFloatDataSize(), 0u);  // No float parameters
}

TEST_F(DisplayListBuilderTest, RecordBoxModel) {
  int32_t index;
  RoundedRectangle rect;
  rect.SetX(1.0);
  rect.SetY(2.0);
  rect.SetWidth(3.0);
  rect.SetHeight(4.0);
  rect.SetRadiusXTopLeft(0.1);

  builder_->RecordBoxModel(rect, index);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();
  const float* float_data_data = display_list.GetContentFloatData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);

  EXPECT_EQ(op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kRecordBox));
  // With optimized AddOperation: [int_count, float_count, param]
  EXPECT_EQ(int_data_data[0], 0);                         // int_count
  EXPECT_EQ(int_data_data[1], 12);                        // float_count
  EXPECT_EQ(display_list.GetContentFloatDataSize(), 12);  // 12 float parameters

  EXPECT_EQ(float_data_data[0], 1.0);
  EXPECT_EQ(float_data_data[1], 2.0);
  EXPECT_EQ(float_data_data[2], 3.0);
  EXPECT_EQ(float_data_data[3], 4.0);
  EXPECT_EQ(float_data_data[4], 0.1f);
  EXPECT_EQ(float_data_data[5], 0);
  EXPECT_EQ(float_data_data[6], 0);
  EXPECT_EQ(float_data_data[7], 0);
  EXPECT_EQ(float_data_data[8], 0);
  EXPECT_EQ(float_data_data[9], 0);
  EXPECT_EQ(float_data_data[10], 0);
  EXPECT_EQ(float_data_data[11], 0);
}

TEST_F(DisplayListBuilderTest, TestCallMarkRootNeedClipBounds) {
  builder_->MarkRootNeedClipBounds();

  DisplayList display_list = builder_->Build();
  EXPECT_TRUE(display_list.RootNeedClipBounds());
}

TEST_F(DisplayListBuilderTest, TestDisplayListReserve) {
  builder_->Reserve(10);

  DisplayList display_list = builder_->Build();
  EXPECT_EQ(display_list.content_data_->ops.capacity(), 10 * 10);
  EXPECT_EQ(display_list.content_data_->int_data.capacity(), 10 * 20);
  EXPECT_EQ(display_list.content_data_->float_data.capacity(), 10 * 20);
}

TEST_F(DisplayListBuilderTest, TestNotCallMarkRootNeedClipBounds) {
  DisplayList display_list = builder_->Build();
  EXPECT_FALSE(display_list.RootNeedClipBounds());
}

TEST_F(DisplayListBuilderTest, LinearGradientOperation) {
  float angle = 45.0f;
  base::Vector<uint32_t> colors = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF};
  base::Vector<float> stops = {0.0f, 0.5f, 1.0f};
  int32_t tiling_index = 1;
  int32_t clip_index = 2;

  builder_->LinearGradient(angle, colors, stops, tiling_index, clip_index);

  DisplayList display_list = builder_->Build();

  const int32_t* op_types_data = display_list.GetContentOpTypesData();
  const int32_t* int_data_data = display_list.GetContentIntData();
  const float* float_data_data = display_list.GetContentFloatData();

  EXPECT_NE(op_types_data, nullptr);
  EXPECT_NE(int_data_data, nullptr);
  EXPECT_NE(float_data_data, nullptr);

  EXPECT_EQ(op_types_data[0],
            static_cast<int32_t>(DisplayListOpType::kLinearGradient));

  // int_data layout: [int_count, float_count, stop_count, colors...,
  // tiling_index, clip_index]
  EXPECT_EQ(int_data_data[0],
            1 + 3 + 2);  // int_count: 1 (stop_count) + 3 (colors) + 2 (indices)
  EXPECT_EQ(int_data_data[1], 1 + 3);  // float_count: 1 (angle) + 3 (stops)

  EXPECT_EQ(int_data_data[2], 3);  // stop_count
  EXPECT_EQ(static_cast<uint32_t>(int_data_data[3]), 0xFFFF0000);
  EXPECT_EQ(static_cast<uint32_t>(int_data_data[4]), 0xFF00FF00);
  EXPECT_EQ(static_cast<uint32_t>(int_data_data[5]), 0xFF0000FF);
  EXPECT_EQ(int_data_data[6], tiling_index);
  EXPECT_EQ(int_data_data[7], clip_index);

  // float_data layout: [angle, positions...]
  EXPECT_FLOAT_EQ(float_data_data[0], angle);
  EXPECT_FLOAT_EQ(float_data_data[1], 0.0f);
  EXPECT_FLOAT_EQ(float_data_data[2], 0.5f);
  EXPECT_FLOAT_EQ(float_data_data[3], 1.0f);
}

}  // namespace tasm
}  // namespace lynx

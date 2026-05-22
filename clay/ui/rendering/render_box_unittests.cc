// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/rendering/render_box.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {

std::unique_ptr<RenderBox> CreateRenderBox(float l, float t, float w, float h) {
  auto res = std::make_unique<RenderBox>();
  res->SetLeft(l);
  res->SetTop(t);
  res->SetWidth(w);
  res->SetHeight(h);
  return res;
}

}  // namespace

TEST(RenderBoxTest, BoxModel) {
  std::unique_ptr<RenderBox> obj = std::make_unique<RenderBox>();
  EXPECT_FALSE(obj->HasBackground());
  EXPECT_FALSE(obj->HasBorder());
  EXPECT_EQ(obj->location(), FloatPoint());
  EXPECT_EQ(obj->Width(), 0.f);
  EXPECT_EQ(obj->Height(), 0.f);

  obj->SetLeft(10.f);
  obj->SetTop(10.f);
  obj->SetWidth(100.f);
  obj->SetHeight(100.f);
  obj->SetPaddingLeft(10.f);
  obj->SetPaddingTop(10.f);
  obj->SetPaddingRight(20.f);
  obj->SetPaddingBottom(20.f);
  EXPECT_EQ(obj->location(), FloatPoint(10.f, 10.f));
  EXPECT_EQ(obj->Width(), 100.f);
  EXPECT_EQ(obj->Height(), 100.f);
  EXPECT_EQ(obj->PaddingLeft(), 10.f);
  EXPECT_EQ(obj->PaddingTop(), 10.f);
  EXPECT_EQ(obj->PaddingRight(), 20.f);
  EXPECT_EQ(obj->PaddingBottom(), 20.f);

  EXPECT_TRUE(FloatRect(obj->BorderBoxRect()) ==
              FloatRect(0.f, 0.f, 100.f, 100.f));
  EXPECT_TRUE(FloatRect(obj->PaddingBoxRect()) ==
              FloatRect(0.f, 0.f, 100.f, 100.f));
  EXPECT_TRUE(FloatRect(obj->PaddingRect()) ==
              FloatRect(10.f, 10.f, 100.f, 100.f));
  EXPECT_TRUE(FloatRect(obj->ContentBoxRect()) ==
              FloatRect(10.f, 10.f, 70.f, 70.f));
  EXPECT_TRUE(FloatRect(obj->ContentRect()) ==
              FloatRect(20.f, 20.f, 70.f, 70.f));
}

TEST(RenderBoxTest, Overflow) {
  // No overflow
  {
    std::unique_ptr<RenderBox> parent = CreateRenderBox(2.f, 4.f, 10.f, 20.f);

    std::unique_ptr<RenderBox> child1 = CreateRenderBox(1.f, 1.f, 1.f, 1.f);
    std::unique_ptr<RenderBox> child2 = CreateRenderBox(8.f, 1.f, 1.f, 1.f);
    std::unique_ptr<RenderBox> child3 = CreateRenderBox(1.f, 18.f, 1.f, 1.f);
    std::unique_ptr<RenderBox> child4 = CreateRenderBox(8.f, 18.f, 1.f, 1.f);
    parent->AddChild(child1.get());
    parent->AddChild(child2.get());
    parent->AddChild(child3.get());
    parent->AddChild(child4.get());
    parent->AddOverflowFromChildren();
    EXPECT_EQ(parent->overflow_rect_.x(), 0);
    EXPECT_EQ(parent->overflow_rect_.y(), 0);
    EXPECT_EQ(parent->overflow_rect_.MaxX(), 10);
    EXPECT_EQ(parent->overflow_rect_.MaxY(), 20);
  }

  {
    std::unique_ptr<RenderBox> parent = CreateRenderBox(2.f, 4.f, 10.f, 20.f);

    std::unique_ptr<RenderBox> child1 = CreateRenderBox(-1.f, 0.f, 1.f, 1.f);
    std::unique_ptr<RenderBox> child2 = CreateRenderBox(9.f, -1.f, 1.f, 1.f);
    std::unique_ptr<RenderBox> child3 = CreateRenderBox(10.f, 19.f, 1.f, 1.f);
    std::unique_ptr<RenderBox> child4 = CreateRenderBox(0.f, 20.f, 1.f, 1.f);
    parent->AddChild(child1.get());
    parent->AddChild(child2.get());
    parent->AddChild(child3.get());
    parent->AddChild(child4.get());
    parent->AddOverflowFromChildren();
    EXPECT_EQ(parent->overflow_rect_.x(), -1);
    EXPECT_EQ(parent->overflow_rect_.y(), -1);
    EXPECT_EQ(parent->overflow_rect_.MaxX(), 11);
    EXPECT_EQ(parent->overflow_rect_.MaxY(), 21);
  }
}

}  // namespace clay

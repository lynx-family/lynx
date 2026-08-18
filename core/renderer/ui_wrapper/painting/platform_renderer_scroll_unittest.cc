// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/starlight/types/layout_types.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define private public
#include "core/renderer/ui_wrapper/painting/platform_renderer_scroll.h"
#undef private

namespace lynx::tasm {
namespace {

using TestFrameInfo = PlatformRendererScroll::FrameInfo;

class TestPlatformRenderer final : public PlatformRendererImpl {
 public:
  explicit TestPlatformRenderer(int id)
      : PlatformRendererImpl(id, PlatformRendererType::kView, base::String()) {}

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override {
    display_list_ = std::move(display_list);
  }

  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&, bool) override {}
  void OnAddChild(PlatformRenderer*, int, bool) override {}
  void OnRemoveFromParent(bool) override {}
  void OnUpdateSubtreeProperties(const DisplayList&) override {}
};

class TestPlatformRendererScroll final : public PlatformRendererScroll {
 public:
  void GenerateContentInfo(const fml::RefPtr<PlatformRendererImpl>& renderer) {
    GenerateContentInfoFromDisplayList(renderer);
  }

  float content_width() const { return content_width_; }
  float content_height() const { return content_height_; }
  const std::vector<ScrollContentOffset>& children_offsets() const {
    return children_offsets_;
  }

 protected:
  void OnContentInfoUpdated(const ContentInfo& content_info) override {
    content_width_ = content_info.width;
    content_height_ = content_info.height;
    children_offsets_.assign(content_info.children_offset.begin(),
                             content_info.children_offset.end());
  }

 private:
  float content_width_ = 0.f;
  float content_height_ = 0.f;
  std::vector<ScrollContentOffset> children_offsets_;
};

const TestFrameInfo* FindFrameById(const std::vector<TestFrameInfo>& frames,
                                   int id) {
  for (const auto& frame : frames) {
    if (frame.id == id) {
      return &frame;
    }
  }
  return nullptr;
}

void ExpectFramePosition(const std::vector<TestFrameInfo>& frames, int id,
                         float expected_x, float expected_y) {
  const auto* frame = FindFrameById(frames, id);
  ASSERT_NE(nullptr, frame);
  EXPECT_FLOAT_EQ(expected_x, frame->left);
  EXPECT_FLOAT_EQ(expected_y, frame->top);
}

/*
 * Logical fragment tree:
 *
 * scroll-view id=1
 * |
 * | -- item-0 id=2 [flattened]
 * |    \
 * |      -- grandchild id=11 [non-flattened / PlatformRenderer]
 * |
 * | -- item-1 id=3 [flattened]
 * |    \
 * |      -- grandchild id=12 [non-flattened / PlatformRenderer]
 * |
 * | -- item-2 id=13 [non-flattened / PlatformRenderer]
 * | -- item-3 id=4 [flattened]
 * |-- item-4 id=5 [flattened]

 * Scroll content coordinate system: 324 x 540
 * padding: left/right = 12, top/bottom = 20
 *
 * (0, 0)
 * +------------------------------------------+
 * |              top padding: 20             |
 * |  (12, 20)  item-0 [flatten]       h=100  |
 * |       +-- (12, 70) grandchild-11 [view]  |
 * |  (12, 120) item-1 [flatten]       h=100  |
 * |       +-- (12, 170) grandchild-12 [view] |
 * |  (12, 220) item-2 [view]          h=100  |
 * |  (12, 320) item-3 [flatten]       h=100  |
 * |  (12, 420) item-4 [flatten]       h=100  |
 * |             bottom padding: 20           |
 * +------------------------------------------+
 *
 * DrawView order and renderer.Children() order:
 *   grandchild-11 -> grandchild-12 -> item-2
 */
TEST(PlatformRendererScrollTest,
     HandlesMixedFlattenedAndPlatformChildrenWithScrollPadding) {
  constexpr float kPaddingLeft = 12.f;
  constexpr float kPaddingRight = 12.f;
  constexpr float kPaddingTop = 20.f;
  constexpr float kPaddingBottom = 20.f;
  constexpr float kItemWidth = 300.f;
  constexpr float kItemHeight = 100.f;
  constexpr size_t kItemCount = 5u;
  // Content width is the sum of all item widths plus padding.
  constexpr float kContentWidth = kPaddingLeft + kItemWidth + kPaddingRight;
  // Content height is the sum of all item heights plus padding.
  constexpr float kContentHeight =
      kPaddingTop + kItemHeight * kItemCount + kPaddingBottom;

  // Create the PlatformRenderer for the scroll-view itself. ID 1 identifies
  // the scroll-view.
  auto root = fml::MakeRefCounted<TestPlatformRenderer>(1);
  // item-0 is flattened and therefore has no PlatformRenderer of its own.
  // Its non-flattened grandchild-11 requires a separate renderer.
  auto first_grandchild = fml::MakeRefCounted<TestPlatformRenderer>(11);
  // item-1 is flattened and therefore has no PlatformRenderer of its own.
  // Its non-flattened grandchild-12 requires a separate renderer.
  auto second_grandchild = fml::MakeRefCounted<TestPlatformRenderer>(12);
  // item-2 is non-flattened and therefore has its own PlatformRenderer.
  auto third_item = fml::MakeRefCounted<TestPlatformRenderer>(13);

  // Build the display list owned by grandchild-11.
  DisplayListBuilder first_grandchild_builder;
  first_grandchild_builder.Begin(11, PlatformRendererType::kView, 0.f, 50.f,
                                 kItemWidth, 50.f);
  first_grandchild_builder.End();
  first_grandchild->UpdateDisplayList(first_grandchild_builder.Build());

  // Build the display list owned by grandchild-12.
  DisplayListBuilder second_grandchild_builder;
  second_grandchild_builder.Begin(12, PlatformRendererType::kView, 0.f, 50.f,
                                  kItemWidth, 50.f);
  second_grandchild_builder.End();
  second_grandchild->UpdateDisplayList(second_grandchild_builder.Build());

  // Build the display list owned by item-2.
  DisplayListBuilder third_item_builder;
  third_item_builder.Begin(13, PlatformRendererType::kView, kPaddingLeft,
                           kPaddingTop + kItemHeight * 2.f, kItemWidth,
                           kItemHeight);
  third_item_builder.End();
  third_item->UpdateDisplayList(third_item_builder.Build());

  // Renderer children follow DrawView order, including DrawViews inside a
  // flattened subtree that the frame visitor skips.
  root->AddChild(first_grandchild);
  root->AddChild(second_grandchild);
  root->AddChild(third_item);

  // Build the display list owned by the scroll-view.
  DisplayListBuilder root_builder;
  root_builder.Begin(1, PlatformRendererType::kScroll, 100.f, 200.f,
                     kContentWidth, 500.f);
  root_builder.BeginScrollContent(1, PlatformRendererType::kScroll);

  // item-0: flattened direct child containing a platform-rendered grandchild.
  root_builder.Begin(2, PlatformRendererType::kView, kPaddingLeft, kPaddingTop,
                     kItemWidth, kItemHeight);
  root_builder.DrawView(11, kPaddingLeft, kPaddingTop + 50.f);
  root_builder.End();

  // item-1: flattened direct child containing a platform-rendered grandchild.
  root_builder.Begin(3, PlatformRendererType::kView, kPaddingLeft,
                     kPaddingTop + kItemHeight, kItemWidth, kItemHeight);
  root_builder.DrawView(12, kPaddingLeft, kPaddingTop + kItemHeight + 50.f);
  root_builder.End();

  // item-2: platform-rendered direct child.
  root_builder.DrawView(13, kPaddingLeft, kPaddingTop + kItemHeight * 2.f);

  // item-3 and item-4: flattened direct children.
  root_builder.Begin(4, PlatformRendererType::kView, kPaddingLeft,
                     kPaddingTop + kItemHeight * 3.f, kItemWidth, kItemHeight);
  root_builder.End();
  root_builder.Begin(5, PlatformRendererType::kView, kPaddingLeft,
                     kPaddingTop + kItemHeight * 4.f, kItemWidth, kItemHeight);
  root_builder.End();

  root_builder.EndScrollContent(kContentWidth, kContentHeight);
  root_builder.End();
  root->UpdateDisplayList(root_builder.Build());

  std::vector<TestFrameInfo> walked_frames;
  float walked_content_width = 0.f;
  float walked_content_height = 0.f;
  const bool walk_completed =
      PlatformRendererScroll::WalkFramesRelativeToContent(
          root, 0.f, 0.f, 0, true,
          [&](const TestFrameInfo& frame, DisplayListOpType op) {
            if (op == DisplayListOpType::kBegin) {
              walked_frames.emplace_back(frame);
            } else if (frame.depth == 0 &&
                       op == DisplayListOpType::kScrollContentEnd) {
              walked_content_width = frame.content_width;
              walked_content_height = frame.content_height;
            }
            return PlatformRendererScroll::WalkAction::kContinue;
          });

  ASSERT_TRUE(walk_completed);
  EXPECT_FLOAT_EQ(kContentWidth, walked_content_width);
  EXPECT_FLOAT_EQ(kContentHeight, walked_content_height);
  ASSERT_EQ(8u, walked_frames.size());

  ExpectFramePosition(walked_frames, 1, 0.f, 0.f);
  ExpectFramePosition(walked_frames, 2, 12.f, 20.f);
  ExpectFramePosition(walked_frames, 11, 12.f, 70.f);
  ExpectFramePosition(walked_frames, 3, 12.f, 120.f);
  ExpectFramePosition(walked_frames, 12, 12.f, 170.f);
  ExpectFramePosition(walked_frames, 13, 12.f, 220.f);
  ExpectFramePosition(walked_frames, 4, 12.f, 320.f);
  ExpectFramePosition(walked_frames, 5, 12.f, 420.f);

  TestPlatformRendererScroll scroll;
  scroll.GenerateContentInfo(root);

  EXPECT_FLOAT_EQ(kContentWidth, scroll.content_width());
  EXPECT_FLOAT_EQ(kContentHeight, scroll.content_height());
  ASSERT_EQ(kItemCount, scroll.children_offsets().size());
  EXPECT_FLOAT_EQ(12.f, scroll.children_offsets()[0].x);
  EXPECT_FLOAT_EQ(20.f, scroll.children_offsets()[0].y);
  EXPECT_FLOAT_EQ(12.f, scroll.children_offsets()[1].x);
  EXPECT_FLOAT_EQ(120.f, scroll.children_offsets()[1].y);
  EXPECT_FLOAT_EQ(12.f, scroll.children_offsets()[2].x);
  EXPECT_FLOAT_EQ(220.f, scroll.children_offsets()[2].y);
  EXPECT_FLOAT_EQ(12.f, scroll.children_offsets()[3].x);
  EXPECT_FLOAT_EQ(320.f, scroll.children_offsets()[3].y);
  EXPECT_FLOAT_EQ(12.f, scroll.children_offsets()[4].x);
  EXPECT_FLOAT_EQ(420.f, scroll.children_offsets()[4].y);
}

}  // namespace
}  // namespace lynx::tasm

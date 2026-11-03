// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#include "core/public/painting_ctx_platform_impl.h"
#undef protected

#include <array>
#include <cmath>
#include <limits>
#include <string>

#include "base/include/debug/lynx_error.h"
#include "base/include/vector.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/dom/fiber/element_utils.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/modifier_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_text_layout.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

class WrapTextLayoutProbe : public TextLayoutMock {
 public:
  LayoutResult Measure(Element*, float width, int width_mode, float,
                       int) override {
    ++calls;
    last_width_mode = width_mode;
    last_width = width;
    const float measured_width =
        width_mode == SLMeasureModeIndefinite ? 300.f : std::min(300.f, width);
    return {measured_width, measured_width < 300 ? 40.f : 20.f, 16.f};
  }
  int calls{0};
  int last_width_mode{-1};
  float last_width{0};
};

fml::RefPtr<ComposeElementHandle> CreateComposeHandle(
    ElementManager* manager,
    ComposeElementKind kind = ComposeElementKind::kView,
    const base::String& tag = base::String()) {
  return fml::AdoptRef<ComposeElementHandle>(
      new ComposeElementHandle(manager, kind, tag));
}

base::Vector<ModifierElement*> CollectModifierFrames(Element* root,
                                                     Element* owner) {
  base::Vector<ModifierElement*> frames;
  for (auto* element = root; element != owner;) {
    if (element == nullptr || !element->is_modifier() ||
        element->GetChildCount() != 1) {
      return {};
    }
    frames.push_back(static_cast<ModifierElement*>(element));
    element = element->GetChildAt(0);
  }
  return frames;
}

lepus::Value MakePaddingChain(size_t frame_count, double value) {
  lepus::Value previous;
  for (size_t index = 0; index < frame_count; ++index) {
    auto background = lepus::Dictionary::Create();
    background->SetValue("op", lepus::Value(3));
    background->SetValue(
        "propertyId",
        lepus::Value(static_cast<int32_t>(kPropertyIDBackgroundColor)));
    background->SetValue("value", lepus::Value("#010101"));
    if (!previous.IsEmpty()) {
      background->SetValue("previous", previous);
    }

    auto padding = lepus::Dictionary::Create();
    padding->SetValue("op", lepus::Value(7));
    padding->SetValue("start", lepus::Value(value + index));
    padding->SetValue("top", lepus::Value(value + index));
    padding->SetValue("end", lepus::Value(value + index));
    padding->SetValue("bottom", lepus::Value(value + index));
    padding->SetValue("previous", lepus::Value(background));
    previous = lepus::Value(padding);
  }
  return previous;
}

lepus::Value AppendTestStyle(const lepus::Value& previous, CSSPropertyID id,
                             const char* value) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(3));
  node->SetValue("propertyId", lepus::Value(static_cast<int32_t>(id)));
  node->SetValue("value", lepus::Value(value));
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value AppendTestPadding(const lepus::Value& previous) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(7));
  for (const auto* side : {"start", "top", "end", "bottom"}) {
    node->SetValue(side, lepus::Value(8.0));
  }
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value AppendTestWeightFill(const lepus::Value& previous,
                                  const lepus::Value& fill) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(13));
  node->SetValue("fill", fill);
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value AppendTestOffset(const lepus::Value& previous, double x,
                              double y) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(14));
  node->SetValue("x", lepus::Value(x));
  node->SetValue("y", lepus::Value(y));
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value AppendTestSizeRange(const lepus::Value& previous, int axes,
                                 double min_width = 0, double min_height = 0,
                                 double max_width = -1,
                                 double max_height = -1) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(15));
  node->SetValue("specifiedAxes", lepus::Value(axes));
  node->SetValue("minWidth", lepus::Value(min_width));
  node->SetValue("minHeight", lepus::Value(min_height));
  node->SetValue("maxWidth", lepus::Value(max_width));
  node->SetValue("maxHeight", lepus::Value(max_height));
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value AppendTestSize(const lepus::Value& previous, double width,
                            double height) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(8));
  node->SetValue("specifiedAxes", lepus::Value(3));
  node->SetValue("width", lepus::Value(width));
  node->SetValue("height", lepus::Value(height));
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value AppendTestWrapWidth(const lepus::Value& previous,
                                 const char* align = "center",
                                 bool unbounded = false) {
  return AppendTestStyle(
      AppendTestStyle(previous, kPropertyIDWidth,
                      unbounded ? "max-content" : "fit-content"),
      kPropertyIDJustifyItems, align);
}

lepus::Value AppendTestWrapHeight(const lepus::Value& previous,
                                  const char* align = "center",
                                  bool unbounded = false) {
  return AppendTestStyle(
      AppendTestStyle(previous, kPropertyIDHeight,
                      unbounded ? "max-content" : "fit-content"),
      kPropertyIDAlignItems, align);
}

lepus::Value AppendTestWrapSize(const lepus::Value& previous,
                                const char* horizontal = "center",
                                const char* vertical = "center",
                                bool unbounded = false) {
  const char* value = unbounded ? "max-content" : "fit-content";
  return AppendTestStyle(
      AppendTestStyle(
          AppendTestStyle(AppendTestStyle(previous, kPropertyIDWidth, value),
                          kPropertyIDHeight, value),
          kPropertyIDJustifyItems, horizontal),
      kPropertyIDAlignItems, vertical);
}

lepus::Value AppendTestWidthFill(const lepus::Value& previous,
                                 double fraction) {
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(9));
  node->SetValue("specifiedAxes", lepus::Value(1));
  node->SetValue("fraction", lepus::Value(fraction));
  node->SetValue("previous", previous);
  return lepus::Value(node);
}

lepus::Value RawStyle(Element* element, CSSPropertyID id) {
  const auto& styles = element->GetCurrentRawInlineStyles();
  if (styles.has_value()) {
    const auto it = styles->find(id);
    if (it != styles->end()) {
      return it->second;
    }
  }
  return lepus::Value();
}

float LeftFromAncestor(starlight::LayoutObject* node,
                       starlight::LayoutObject* ancestor) {
  float left = 0;
  while (node != ancestor && node->ParentLayoutObject()) {
    auto* parent = node->ParentLayoutObject();
    left += node->GetBoundLeftFrom(parent, starlight::BoundType::kBorder,
                                   starlight::BoundType::kBorder);
    node = parent;
  }
  EXPECT_EQ(ancestor, node);
  return left;
}

float TopFromAncestor(starlight::LayoutObject* node,
                      starlight::LayoutObject* ancestor) {
  float top = 0;
  while (node != ancestor && node->ParentLayoutObject()) {
    auto* parent = node->ParentLayoutObject();
    top += node->GetBoundTopFrom(parent, starlight::BoundType::kBorder,
                                 starlight::BoundType::kBorder);
    node = parent;
  }
  EXPECT_EQ(ancestor, node);
  return top;
}

Element* WrappedContent(Element* root) {
  while (root->is_modifier()) root = root->GetChildAt(0);
  return root;
}

}  // namespace

class ModifierElementTest : public FiberElementTest {
 protected:
  fml::RefPtr<PageElement> CreateComposePage(float scale = 1,
                                             double physical_scale = 1) {
    PageOptions options;
    options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
    tasm->SetPageOptions(options);
    manager->GetConfig()->SetEnableCSSInheritance(true);
    // Compose templates target SDK 3.2, with CSS relative positioning enabled.
    manager->GetConfig()->SetQuirksModeByVersion(base::Version(3, 2));
    manager->SetConfig(manager->GetConfig());
    manager->GetLynxEnvConfig() =
        LynxEnvConfig(kWidth, kHeight, scale, physical_scale);
    manager->SetLayoutTick(
        [this](const auto& pipeline) { manager->RequestLayout(pipeline); });
    auto page = manager->CreateFiberPage("page", 0);
    page->MarkAsComposeLayout();
    page->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
    manager->SetFiberPageElement(page);
    return page;
  }

  void FlushAndLayout(const fml::RefPtr<PageElement>& page) {
    page->FlushActionsAsRoot();
    page->Layout(std::make_shared<PipelineOptions>());
  }
};

TEST_P(ModifierElementTest,
       WrapContentHeightAndSizeAlignAndPreserveOuterBounds) {
  for (bool both_axes : {false, true}) {
    for (bool unbounded : {false, true}) {
      for (bool rtl : {false, true}) {
        for (float scale : {1.f, 2.f}) {
          auto page = CreateComposePage(1, scale);
          for (const auto* horizontal : {"start", "center", "end"}) {
            for (const auto* vertical : {"start", "center", "end"}) {
              for (float height : {20.f, 21.f, 120.f, 121.f}) {
                auto handle = CreateComposeHandle(manager);
                auto prefix = AppendTestSize({}, 200, 80);
                auto source =
                    both_axes
                        ? AppendTestWrapSize(prefix, horizontal, vertical,
                                             unbounded)
                        : AppendTestWrapHeight(prefix, vertical, unbounded);
                source = AppendTestSize(source, 301, height);
                ASSERT_TRUE(ComposeModifierApplicator::Apply(handle.get(),
                                                             source, nullptr)
                                .success);
                handle->mount_root()->SetStyle(
                    kPropertyIDDirection, lepus::Value(rtl ? "rtl" : "ltr"));
                page->InsertNode(handle->mount_root());
                FlushAndLayout(page);
                auto* frame = handle->mount_root()->slnode();
                auto* child = handle->content_element()->slnode();
                const float width = both_axes && unbounded ? 301 : 200;
                const float child_height =
                    unbounded ? height : std::min(height, 80.f);
                EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
                EXPECT_FLOAT_EQ(80, frame->GetBorderBoundHeight());
                EXPECT_FLOAT_EQ(width, child->GetBorderBoundWidth());
                EXPECT_FLOAT_EQ(child_height, child->GetBorderBoundHeight());
                float x = std::string(horizontal) == "center"
                              ? (200 - width) / 2
                          : std::string(horizontal) == "end" ? 200 - width
                                                             : 0;
                if (rtl) x = 200 - width - x;
                x = std::floor(x * scale + 0.5f) / scale;
                float y = std::string(vertical) == "center"
                              ? (80 - child_height) / 2
                          : std::string(vertical) == "end" ? 80 - child_height
                                                           : 0;
                y = std::floor(y * scale + 0.5f) / scale;
                EXPECT_FLOAT_EQ(x, LeftFromAncestor(child, frame));
                EXPECT_FLOAT_EQ(y, TopFromAncestor(child, frame));
                // A two-axis wrap is one boundary, not two nested axis wraps.
                EXPECT_EQ(2u,
                          CollectModifierFrames(handle->mount_root().get(),
                                                handle->content_element().get())
                              .size());
                EXPECT_EQ(nullptr, frame->GetSLMeasureFunc());
                EXPECT_EQ(nullptr, frame->GetSLAlignmentFunc());
                page->RemoveNode(handle->mount_root());
              }
            }
          }
        }
      }
    }
  }
}

TEST_P(ModifierElementTest,
       WrapContentHeightAndSizePreserveOtherAxisConstraints) {
  auto page = CreateComposePage();
  for (bool both_axes : {false, true})
    for (bool unbounded : {false, true}) {
      auto handle = CreateComposeHandle(manager);
      auto source = both_axes
                        ? AppendTestWrapSize({}, "center", "center", unbounded)
                        : AppendTestWrapHeight({}, "center", unbounded);
      ASSERT_TRUE(
          ComposeModifierApplicator::Apply(handle.get(), source, nullptr)
              .success);
      page->InsertNode(handle->mount_root());
      page->FlushActionsAsRoot();
      auto* child = handle->content_element()->slnode();
      starlight::ComposeConstraints observed;
      void* context = child->GetContext();
      child->SetContext(&observed);
      child->SetSLComposeMeasureFunc(
          [](void* context, const starlight::ComposeConstraints& range, bool) {
            *static_cast<starlight::ComposeConstraints*>(context) = range;
            return FloatSize(range.ConstrainWidth(60),
                             range.ConstrainHeight(20));
          });
      auto* frame = handle->mount_root()->slnode();
      frame->UpdateMeasureWithComposeConstraints({120, 200, 50, 80}, true);
      child->SetContext(context);
      child->SetSLComposeMeasureFunc(nullptr);
      EXPECT_FLOAT_EQ(both_axes ? 0 : 120, observed.min_width);
      EXPECT_FLOAT_EQ(0, observed.min_height);
      EXPECT_EQ(both_axes && unbounded, std::isinf(observed.max_width));
      EXPECT_EQ(unbounded, std::isinf(observed.max_height));
      EXPECT_FLOAT_EQ(120, frame->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(50, frame->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(both_axes ? 60 : 120, child->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(20, child->GetBorderBoundHeight());
      EXPECT_EQ(1u, CollectModifierFrames(handle->mount_root().get(),
                                          handle->content_element().get())
                        .size());
      page->RemoveNode(handle->mount_root());
    }
}

TEST_P(ModifierElementTest,
       WrapContentHeightAndSizeKeepPaddingRangeAndFillOrder) {
  auto fixed = AppendTestSize({}, 200, 80);
  auto width_only = AppendTestSize({}, 200, 0);
  width_only.Table()->SetValue("specifiedAxes", lepus::Value(1));
  struct Case {
    lepus::Value source;
    float width, height, x, y;
  };
  const Case cases[] = {
      {AppendTestSize(AppendTestWrapHeight(fixed), 60, 20), 200, 20, 0, 30},
      {AppendTestSize(AppendTestWrapSize(fixed), 60, 20), 60, 20, 70, 30},
      {AppendTestSize(AppendTestWrapHeight(AppendTestPadding(fixed)), 60, 20),
       184, 20, 8, 30},
      {AppendTestSize(AppendTestPadding(AppendTestWrapSize(fixed)), 60, 20), 60,
       20, 70, 30},
      {AppendTestSize(
           AppendTestWrapSize(AppendTestSizeRange(fixed, 3, 100, 40)), 60, 20),
       60, 20, 70, 30},
      {AppendTestSize(
           AppendTestSizeRange(AppendTestWrapSize(fixed), 3, 100, 40), 60, 20),
       100, 40, 50, 20},
      {AppendTestSize(AppendTestWrapHeight(AppendTestWrapWidth(fixed)), 60, 20),
       60, 20, 70, 30},
      {AppendTestSize(AppendTestWrapWidth(AppendTestWrapHeight(fixed)), 60, 20),
       60, 20, 70, 30},
      {AppendTestSize(AppendTestWrapSize(
                          AppendTestSize(AppendTestWrapSize(fixed), 60, 40)),
                      20, 10),
       20, 10, 90, 35},
      {AppendTestWidthFill(AppendTestWrapSize(fixed), 0.5), 100, 0, 50, 40},
      {AppendTestWrapHeight(width_only), 200, 0, 0, 0},
  };
  auto page = CreateComposePage();
  int index = 0;
  for (const auto& test : cases) {
    SCOPED_TRACE(index++);
    auto handle = CreateComposeHandle(manager);
    ASSERT_TRUE(
        ComposeModifierApplicator::Apply(handle.get(), test.source, nullptr)
            .success);
    page->InsertNode(handle->mount_root());
    FlushAndLayout(page);
    auto* child = handle->content_element()->slnode();
    auto* frame = handle->mount_root()->slnode();
    EXPECT_FLOAT_EQ(test.width, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(test.height, child->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(test.x, LeftFromAncestor(child, frame));
    EXPECT_FLOAT_EQ(test.y, TopFromAncestor(child, frame));
    page->RemoveNode(handle->mount_root());
  }
  // Full and fractional fills stop independently at each wrapped axis.
  for (bool both_axes : {false, true})
    for (double fraction : {1., 0.5}) {
      auto handle = CreateComposeHandle(manager);
      auto wrapped =
          both_axes ? AppendTestWrapSize(fixed) : AppendTestWrapHeight(fixed);
      auto fill = AppendTestWidthFill(wrapped, fraction);
      fill.Table()->SetValue("specifiedAxes", lepus::Value(3));
      ASSERT_TRUE(ComposeModifierApplicator::Apply(handle.get(), fill, nullptr)
                      .success);
      page->InsertNode(handle->mount_root());
      FlushAndLayout(page);
      auto* child = handle->content_element()->slnode();
      EXPECT_FLOAT_EQ(both_axes ? 200 * fraction : 200,
                      child->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(80 * fraction, child->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(200,
                      handle->mount_root()->slnode()->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(80,
                      handle->mount_root()->slnode()->GetBorderBoundHeight());
      page->RemoveNode(handle->mount_root());
    }
}

TEST_P(ModifierElementTest, WrapContentHeightAndSizeReplaceCloneAndRemove) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  for (bool both_axes : {false, true, false}) {
    ASSERT_TRUE(
        ComposeModifierApplicator::Apply(
            handle.get(),
            AppendTestSize(both_axes
                               ? AppendTestWrapSize(AppendTestSize({}, 200, 80),
                                                    "end", "end", true)
                               : AppendTestWrapHeight(
                                     AppendTestSize({}, 200, 80), "end", true),
                           300, 120),
            nullptr)
            .success);
    if (!handle->mount_root()->parent()) page->InsertNode(handle->mount_root());
    FlushAndLayout(page);
    EXPECT_EQ(owner, handle->content_element());
    EXPECT_FLOAT_EQ(
        -40, TopFromAncestor(owner->slnode(), handle->mount_root()->slnode()));
    for (bool resolved : {false, true}) {
      auto clone = TreeResolver::CloneElements(
          handle->mount_root(), handle->mount_root()->style_sheet_manager(),
          resolved, TreeResolver::CloningDepth::kTree);
      page->InsertNode(clone);
      FlushAndLayout(page);
      auto* content = WrappedContent(clone.get())->slnode();
      EXPECT_FLOAT_EQ(both_axes ? 300 : 200, content->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(120, content->GetBorderBoundHeight());
      EXPECT_FLOAT_EQ(-40, TopFromAncestor(content, clone->slnode()));
      page->RemoveNode(clone);
    }
  }
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestSize({}, 60, 20), nullptr)
                  .success);
  FlushAndLayout(page);
  EXPECT_EQ(owner, handle->mount_root());
  EXPECT_FLOAT_EQ(20, owner->slnode()->GetBorderBoundHeight());
}

TEST_P(ModifierElementTest, WrapContentHeightAndSizeRejectInvalidAlignments) {
  auto handle = CreateComposeHandle(manager);
  auto root = handle->mount_root();
  for (auto source : {AppendTestWrapHeight({}, "invalid"),
                      AppendTestWrapSize({}, "invalid", "end"),
                      AppendTestWrapSize({}, "end", "invalid")}) {
    EXPECT_FALSE(ComposeModifierApplicator::Apply(handle.get(), source, nullptr)
                     .success);
    EXPECT_EQ(root, handle->mount_root());
  }
}

TEST_P(ModifierElementTest,
       WrapContentHeightAndSizeMeasureTextImageRowAndColumn) {
  platform_impl_->text_layout_impl_ = std::make_unique<WrapTextLayoutProbe>();
  auto page = CreateComposePage();
  for (int kind = 0; kind < 4; ++kind) {
    for (bool both_axes : {false, true})
      for (bool unbounded : {false, true}) {
        SCOPED_TRACE(kind);
        auto handle = CreateComposeHandle(
            manager, kind == 0   ? ComposeElementKind::kText
                     : kind == 1 ? ComposeElementKind::kImage
                                 : ComposeElementKind::kView);
        auto source = both_axes
                          ? AppendTestWrapSize(AppendTestSize({}, 200, 30),
                                               "end", "end", unbounded)
                          : AppendTestWrapHeight(AppendTestSize({}, 200, 30),
                                                 "end", unbounded);
        if (kind == 1) source = AppendTestSize(source, 300, 120);
        ASSERT_TRUE(
            ComposeModifierApplicator::Apply(handle.get(), source, nullptr)
                .success);
        auto content = handle->content_element();
        if (kind == 0)
          content->SetAttribute("text", lepus::Value("wrapped text"));
        if (kind == 1)
          content->SetAttribute("src", lepus::Value("wrap-content.png"));
        if (kind >= 2) {
          content->SetStyle(kPropertyIDDisplay, lepus::Value("linear"));
          content->SetStyle(kPropertyIDLinearOrientation,
                            lepus::Value(kind == 2 ? "row" : "column"));
          for (int i = 0; i < 2; ++i) {
            auto child = CreateComposeHandle(manager);
            ASSERT_TRUE(ComposeModifierApplicator::Apply(
                            child.get(),
                            AppendTestSize({}, kind == 2 ? 150 : 300, 60),
                            nullptr)
                            .success);
            content->InsertNode(child->mount_root());
          }
        }
        page->InsertNode(handle->mount_root());
        FlushAndLayout(page);
        auto* frame = handle->mount_root()->slnode();
        auto* child = content->slnode();
        const float width = both_axes && unbounded ? 300 : 200;
        const float natural_height = kind == 0   ? (width == 300 ? 20 : 40)
                                     : kind == 2 ? 60
                                                 : 120;
        const float height =
            unbounded ? natural_height : std::min(natural_height, 30.f);
        EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(30, frame->GetBorderBoundHeight());
        EXPECT_FLOAT_EQ(width, child->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(height, child->GetBorderBoundHeight());
        EXPECT_FLOAT_EQ(200 - width, LeftFromAncestor(child, frame));
        EXPECT_FLOAT_EQ(30 - height, TopFromAncestor(child, frame));
        if (kind >= 2) EXPECT_EQ(2u, content->GetChildCount());
        page->RemoveNode(handle->mount_root());
      }
  }
}

TEST_P(ModifierElementTest, WrapContentHeightAndSizeDoNotMoveColumnSibling) {
  auto page = CreateComposePage();
  auto column = CreateComposeHandle(manager)->content_element();
  column->SetStyle(kPropertyIDDisplay, lepus::Value("linear"));
  column->SetStyle(kPropertyIDLinearOrientation, lepus::Value("column"));
  for (bool both_axes : {false, true}) {
    auto handle = CreateComposeHandle(manager);
    auto tail = CreateComposeHandle(manager);
    auto prefix = AppendTestSize({}, 200, 80);
    auto wrap = both_axes ? AppendTestWrapSize(prefix, "center", "center", true)
                          : AppendTestWrapHeight(prefix, "center", true);
    ASSERT_TRUE(ComposeModifierApplicator::Apply(
                    handle.get(), AppendTestSize(wrap, 300, 120), nullptr)
                    .success);
    ASSERT_TRUE(ComposeModifierApplicator::Apply(
                    tail.get(), AppendTestSize({}, 20, 20), nullptr)
                    .success);
    column->InsertNode(handle->mount_root());
    column->InsertNode(tail->mount_root());
    page->InsertNode(column);
    FlushAndLayout(page);
    EXPECT_FLOAT_EQ(80, tail->mount_root()
                            ->slnode()
                            ->GetBorderBoundTopFromParentPaddingBound());
    EXPECT_FLOAT_EQ(80, handle->mount_root()->slnode()->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(
        120, handle->content_element()->slnode()->GetBorderBoundHeight());
    page->RemoveNode(column);
    column->RemoveNode(handle->mount_root());
    column->RemoveNode(tail->mount_root());
  }
}

TEST_P(ModifierElementTest, WrapContentWidthAlignsWithoutChangingRowSlot) {
  for (bool unbounded : {false, true}) {
    for (bool rtl : {false, true}) {
      for (const auto* align : {"start", "center", "end"}) {
        for (float requested_width : {60.f, 61.f, 300.f, 301.f}) {
          auto page = CreateComposePage();
          auto row = CreateComposeHandle(manager)->content_element();
          row->SetStyle(kPropertyIDDisplay, lepus::Value("linear"));
          row->SetStyle(kPropertyIDLinearOrientation, lepus::Value("row"));
          row->SetStyle(kPropertyIDWidth, lepus::Value("300px"));
          row->SetStyle(kPropertyIDDirection,
                        lepus::Value(rtl ? "rtl" : "ltr"));
          auto handle = CreateComposeHandle(manager);
          auto owner = handle->content_element();
          auto tail = CreateComposeHandle(manager);
          auto source = AppendTestWrapWidth(AppendTestSize({}, 200, 80), align,
                                            unbounded);
          source = AppendTestSize(source, requested_width, 20);
          ASSERT_TRUE(
              ComposeModifierApplicator::Apply(handle.get(), source, nullptr)
                  .success);
          ASSERT_TRUE(ComposeModifierApplicator::Apply(
                          tail.get(), AppendTestSize({}, 20, 20), nullptr)
                          .success);
          row->InsertNode(handle->mount_root());
          row->InsertNode(tail->mount_root());
          page->InsertNode(row);
          FlushAndLayout(page);

          auto* frame = handle->mount_root()->slnode();
          auto* child = owner->slnode();
          const float width =
              unbounded ? requested_width : std::min(requested_width, 200.f);
          float x = std::string(align) == "center" ? (200 - width) / 2
                    : std::string(align) == "end"  ? 200 - width
                                                   : 0;
          if (rtl) x = 200 - width - x;
          x = std::floor(x + 0.5f);
          EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
          EXPECT_FLOAT_EQ(80, frame->GetBorderBoundHeight());
          EXPECT_FLOAT_EQ(width, child->GetBorderBoundWidth());
          EXPECT_FLOAT_EQ(80, child->GetBorderBoundHeight());
          EXPECT_FLOAT_EQ(x, LeftFromAncestor(child, frame));
          EXPECT_EQ(frame, child->ParentLayoutObject()->ParentLayoutObject());
          EXPECT_EQ(frame->FirstChild(), child->ParentLayoutObject());
          EXPECT_EQ(nullptr, frame->GetSLMeasureFunc());
          EXPECT_EQ(nullptr, frame->GetSLAlignmentFunc());
          EXPECT_FLOAT_EQ(rtl ? 80 : 200,
                          tail->mount_root()
                              ->slnode()
                              ->GetBorderBoundLeftFromParentPaddingBound());
          EXPECT_FALSE(
              handle->mount_root()->computed_css_style()->IsOverflowHidden());
          page->RemoveNode(row);
        }
      }
    }
  }
}

TEST_P(ModifierElementTest, WrapContentWidthPreservesHeightAndParentRanges) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestSize(AppendTestWrapWidth({}), 60, 20),
                  nullptr)
                  .success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  auto* frame = handle->mount_root()->slnode();
  auto* child = handle->content_element()->slnode();
  for (const auto& constraints :
       {starlight::ComposeConstraints{120, 200, 50, 90},
        starlight::ComposeConstraints{0, 200, 10, 90},
        starlight::ComposeConstraints{200, 200, 50, 50}}) {
    frame->UpdateMeasureWithComposeConstraints(constraints, true);
    EXPECT_FLOAT_EQ(std::max(60.f, constraints.min_width),
                    frame->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(60, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(std::max(20.f, constraints.min_height),
                    child->GetBorderBoundHeight());
    EXPECT_FLOAT_EQ(child->GetBorderBoundHeight(),
                    frame->GetBorderBoundHeight());
  }
}

TEST_P(ModifierElementTest, WrapContentWidthCentersOnPhysicalPixels) {
  for (float physical_scale : {1.f, 2.f}) {
    auto page = CreateComposePage(1, physical_scale);
    for (bool rtl : {false, true}) {
      for (float width : {61.f, 301.f}) {
        auto handle = CreateComposeHandle(manager);
        auto source = AppendTestSize(
            AppendTestWrapWidth(AppendTestSize({}, 200, 40), "center", true),
            width, 20);
        ASSERT_TRUE(
            ComposeModifierApplicator::Apply(handle.get(), source, nullptr)
                .success);
        handle->mount_root()->SetStyle(kPropertyIDDirection,
                                       lepus::Value(rtl ? "rtl" : "ltr"));
        page->InsertNode(handle->mount_root());
        FlushAndLayout(page);
        const float x = std::floor((200 - width) * physical_scale / 2 + 0.5f) /
                        physical_scale;
        EXPECT_FLOAT_EQ(x, LeftFromAncestor(handle->content_element()->slnode(),
                                            handle->mount_root()->slnode()));
        page->RemoveNode(handle->mount_root());
      }
    }
  }
}

TEST_P(ModifierElementTest,
       WrapContentWidthMeasuresNaturalContentWithinHeight) {
  auto page = CreateComposePage();
  for (bool unbounded : {false, true}) {
    auto handle = CreateComposeHandle(manager);
    ASSERT_TRUE(ComposeModifierApplicator::Apply(
                    handle.get(),
                    AppendTestWrapWidth(AppendTestSize({}, 200, 80), "center",
                                        unbounded),
                    nullptr)
                    .success);
    page->InsertNode(handle->mount_root());
    page->FlushActionsAsRoot();
    auto* child = handle->content_element()->slnode();
    child->SetSLComposeMeasureFunc(
        [](void*, const starlight::ComposeConstraints& constraints, bool) {
          EXPECT_FLOAT_EQ(0, constraints.min_width);
          EXPECT_FLOAT_EQ(80, constraints.min_height);
          EXPECT_FLOAT_EQ(80, constraints.max_height);
          return FloatSize(constraints.ConstrainWidth(300),
                           constraints.ConstrainHeight(20));
        });
    page->Layout(std::make_shared<PipelineOptions>());
    auto* frame = handle->mount_root()->slnode();
    EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(unbounded ? 300 : 200, child->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(80, child->GetBorderBoundHeight());
    page->RemoveNode(handle->mount_root());
  }
}

TEST_P(ModifierElementTest, WrapContentWidthKeepsSizePaddingAndFillOrder) {
  const auto fixed_width = AppendTestSize({}, 200, 40);
  const struct {
    lepus::Value source;
    float outer_width;
    float content_width;
    float content_x;
  } cases[] = {
      {AppendTestSize(AppendTestWrapWidth(fixed_width), 60, 20), 200, 60, 70},
      {AppendTestSize(AppendTestWrapWidth(AppendTestPadding(fixed_width)), 60,
                      20),
       200, 60, 70},
      {AppendTestSize(AppendTestPadding(AppendTestWrapWidth(fixed_width)), 60,
                      20),
       200, 60, 70},
      {AppendTestSize(
           AppendTestWrapWidth(AppendTestSize(AppendTestPadding({}), 200, 40)),
           60, 20),
       216, 60, 78},
      {AppendTestSize(AppendTestWrapWidth(AppendTestSize(
                          AppendTestWrapWidth(fixed_width), 60, 20)),
                      20, 10),
       200, 20, 90},
      {AppendTestSize(
           AppendTestSizeRange(AppendTestWrapWidth(fixed_width), 1, 80), 60,
           20),
       200, 80, 60},
      {AppendTestSize(AppendTestWrapWidth(AppendTestSizeRange({}, 1, 120)), 60,
                      20),
       120, 60, 30},
      {AppendTestWidthFill(AppendTestPadding(AppendTestWrapWidth(fixed_width)),
                           1),
       200, 184, 8},
      {AppendTestWidthFill(AppendTestWrapWidth(fixed_width), 0.5), 200, 100,
       50},
  };
  auto page = CreateComposePage();
  int case_index = 0;
  for (const auto& test : cases) {
    SCOPED_TRACE(case_index++);
    auto handle = CreateComposeHandle(manager);
    ASSERT_TRUE(
        ComposeModifierApplicator::Apply(handle.get(), test.source, nullptr)
            .success);
    page->InsertNode(handle->mount_root());
    FlushAndLayout(page);
    auto* frame = handle->mount_root()->slnode();
    auto* child = handle->content_element()->slnode();
    EXPECT_FLOAT_EQ(test.outer_width, frame->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(test.content_width, child->GetBorderBoundWidth());
    float x = 0;
    for (auto* node = child; node != frame; node = node->ParentLayoutObject()) {
      ASSERT_NE(node->ParentLayoutObject(), nullptr);
      x += node->GetBoundLeftFrom(node->ParentLayoutObject(),
                                  starlight::BoundType::kBorder,
                                  starlight::BoundType::kBorder);
    }
    EXPECT_FLOAT_EQ(test.content_x, x);
    page->RemoveNode(handle->mount_root());
  }
}

TEST_P(ModifierElementTest, WrapContentWidthClonesAndClearsOnRemoval) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(),
                  AppendTestSize(AppendTestWrapWidth(
                                     AppendTestSize({}, 200, 40), "end", true),
                                 300, 20),
                  nullptr)
                  .success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  for (bool resolved : {false, true}) {
    auto clone = TreeResolver::CloneElements(
        handle->mount_root(), handle->mount_root()->style_sheet_manager(),
        resolved, TreeResolver::CloningDepth::kTree);
    page->InsertNode(clone);
    FlushAndLayout(page);
    EXPECT_FLOAT_EQ(200, clone->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(
        300, WrappedContent(clone.get())->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(-100,
                    LeftFromAncestor(WrappedContent(clone.get())->slnode(),
                                     clone->slnode()));
    starlight::LayoutComputedStyle copy(1);
    copy.CopyFrom(*WrappedContent(clone.get())
                       ->slnode()
                       ->ParentLayoutObject()
                       ->GetCSSStyle());
    EXPECT_TRUE(copy.GetWidth().IsMaxContent());
    copy.Reset();
    EXPECT_TRUE(copy.GetWidth().IsAuto());
    page->RemoveNode(clone);
  }
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestSize({}, 60, 20), nullptr)
                  .success);
  FlushAndLayout(page);
  EXPECT_EQ(owner, handle->mount_root());
  EXPECT_FLOAT_EQ(60, owner->slnode()->GetBorderBoundWidth());
  EXPECT_FALSE(owner->slnode()->GetCSSStyle()->GetWidth().IsIntrinsic());
}

TEST_P(ModifierElementTest, WrapContentWidthKeepsHeightOnSameFrame) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto height = AppendTestSize({}, 0, 40);
  height.Table()->SetValue("specifiedAxes", lepus::Value(2));
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(),
                  AppendTestSize(AppendTestWrapWidth(height), 60, 20), nullptr)
                  .success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  auto* frame = handle->mount_root()->slnode();
  auto* child = handle->content_element()->slnode();
  EXPECT_EQ(frame, child->ParentLayoutObject());
  EXPECT_FLOAT_EQ(60, frame->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(40, frame->GetBorderBoundHeight());
  EXPECT_FLOAT_EQ(40, child->GetBorderBoundHeight());
}

TEST_P(ModifierElementTest, WrapContentWidthReplacesMountedFrames) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  for (bool unbounded : {false, true, false}) {
    ASSERT_TRUE(
        ComposeModifierApplicator::Apply(
            handle.get(),
            AppendTestSize(AppendTestWrapWidth(AppendTestSize({}, 200, 40),
                                               "end", unbounded),
                           300, 20),
            nullptr)
            .success);
    if (!handle->mount_root()->parent()) page->InsertNode(handle->mount_root());
    FlushAndLayout(page);
    EXPECT_EQ(owner, handle->content_element());
    EXPECT_EQ(handle->mount_root().get(), page->GetChildAt(0));
    EXPECT_FLOAT_EQ(200, handle->mount_root()->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(unbounded ? 300 : 200,
                    owner->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(
        unbounded ? -100 : 0,
        LeftFromAncestor(owner->slnode(), handle->mount_root()->slnode()));
  }
}

TEST_P(ModifierElementTest, WrapContentWidthRejectsInvalidIrBeforeMutation) {
  auto handle = CreateComposeHandle(manager);
  auto root = handle->mount_root();
  for (const auto& source :
       {AppendTestWrapWidth({}, "invalid"), AppendTestWrapWidth({}, "left")}) {
    EXPECT_FALSE(ComposeModifierApplicator::Apply(handle.get(), source, nullptr)
                     .success);
    EXPECT_EQ(root, handle->mount_root());
  }
  auto source = AppendTestWrapWidth({});
  source.Table()->SetValue("value", lepus::Value(1));
  EXPECT_FALSE(
      ComposeModifierApplicator::Apply(handle.get(), source, nullptr).success);
  EXPECT_EQ(root, handle->mount_root());
}

TEST_P(ModifierElementTest, WrapContentWidthMeasuresRealTextElement) {
  auto probe = std::make_unique<WrapTextLayoutProbe>();
  auto* measured_text = probe.get();
  platform_impl_->text_layout_impl_ = std::move(probe);
  auto page = CreateComposePage();
  for (bool unbounded : {false, true}) {
    for (bool rtl : {false, true}) {
      for (const auto* align : {"start", "center", "end"}) {
        auto handle = CreateComposeHandle(manager, ComposeElementKind::kText);
        auto width = AppendTestSize({}, 200, 0);
        width.Table()->SetValue("specifiedAxes", lepus::Value(1));
        ASSERT_TRUE(ComposeModifierApplicator::Apply(
                        handle.get(),
                        AppendTestWrapWidth(width, align, unbounded), nullptr)
                        .success);
        auto owner = handle->content_element();
        ASSERT_TRUE(owner->is_text());
        owner->SetAttribute("text", lepus::Value("wrap content measurement"));
        handle->mount_root()->SetStyle(kPropertyIDDirection,
                                       lepus::Value(rtl ? "rtl" : "ltr"));
        page->InsertNode(handle->mount_root());
        const int previous_calls = measured_text->calls;
        FlushAndLayout(page);
        EXPECT_GT(measured_text->calls, previous_calls);
        EXPECT_EQ(unbounded ? SLMeasureModeIndefinite : SLMeasureModeAtMost,
                  measured_text->last_width_mode);
        if (!unbounded) EXPECT_FLOAT_EQ(200, measured_text->last_width);
        auto* frame = handle->mount_root()->slnode();
        auto* content = owner->slnode();
        EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(unbounded ? 300 : 200, content->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(unbounded ? 20 : 40, frame->GetBorderBoundHeight());
        float x = !unbounded                       ? 0
                  : std::string(align) == "center" ? -50
                  : std::string(align) == "end"    ? -100
                                                   : 0;
        if (rtl) x = 200 - content->GetBorderBoundWidth() - x;
        EXPECT_FLOAT_EQ(x, LeftFromAncestor(content, frame));
        EXPECT_EQ(frame, content->ParentLayoutObject()->ParentLayoutObject());
        EXPECT_EQ("wrap content measurement",
                  static_cast<TextElement*>(owner.get())->content().str());
        page->RemoveNode(handle->mount_root());
      }
    }
  }
}

TEST_P(ModifierElementTest, WrapContentWidthUsesExistingCssStyleBindings) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto source = AppendTestSize(
      AppendTestWrapWidth(AppendTestSize({}, 200, 40), "end", true), 300, 20);
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), source, nullptr).success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  EXPECT_FLOAT_EQ(200, handle->mount_root()->slnode()->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(300,
                  handle->content_element()->slnode()->GetBorderBoundWidth());

  auto mts_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  mts_runtime->Initialize();
  mts_runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  Renderer::RegisterBuiltin(mts_runtime.get(), ArchOption::FIBER_ARCH);
  mts_runtime->SetGlobalData("wrapPage", lepus::Value(page));
  mts_runtime->SetGlobalData("wrapPageId", lepus::Value(page->impl_id()));
  mts_runtime->SetGlobalData("wrapWidthId",
                             lepus::Value(static_cast<int>(kPropertyIDWidth)));
  mts_runtime->SetGlobalData(
      "wrapJustifyId", lepus::Value(static_cast<int>(kPropertyIDJustifyItems)));
  const std::string script = R"JS(
    globalThis.wrapped = __CreateCompose(wrapPageId, 1);
    __SetComposeModifier(wrapped, {
      op: 8, specifiedAxes: 3, width: 300, height: 20, previous: {
        op: 3, propertyId: wrapJustifyId, value: 'center', previous: {
          op: 3, propertyId: wrapWidthId, value: 'max-content', previous: {
            op: 8, specifiedAxes: 3, width: 200, height: 40
          }
        }
      }
    });
    __InsertElementAt(wrapPage, wrapped, 1);
    wrapped;
  )JS";
  lepus::Value value;
  ASSERT_TRUE(runtime::MTSRuntime::ToQuickContext(mts_runtime.get())
                  ->EvalBuf(script.c_str(), script.size(), value,
                            "wrap-style-test.js"));
  auto root = GetComposeMountRootOrFiberElementFromValue(value);
  ASSERT_TRUE(root->is_modifier());
  FlushAndLayout(page);
  EXPECT_FLOAT_EQ(200, root->slnode()->GetBorderBoundWidth());
  auto* child = WrappedContent(root.get())->slnode();
  EXPECT_FLOAT_EQ(300, child->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(-50, LeftFromAncestor(child, root->slnode()));
}

TEST_P(ModifierElementTest, WrapContentWidthPreservesImageContentAndSize) {
  auto page = CreateComposePage();
  for (bool unbounded : {false, true}) {
    for (bool rtl : {false, true}) {
      for (const auto* align : {"start", "center", "end"}) {
        auto handle = CreateComposeHandle(manager, ComposeElementKind::kImage);
        ASSERT_TRUE(
            ComposeModifierApplicator::Apply(
                handle.get(),
                AppendTestSize(AppendTestWrapWidth(AppendTestSize({}, 200, 40),
                                                   align, unbounded),
                               300, 20),
                nullptr)
                .success);
        auto owner = handle->content_element();
        ASSERT_TRUE(owner->is_image());
        owner->SetAttribute("src", lepus::Value("wrap-content.png"));
        handle->mount_root()->SetStyle(kPropertyIDDirection,
                                       lepus::Value(rtl ? "rtl" : "ltr"));
        page->InsertNode(handle->mount_root());
        FlushAndLayout(page);
        auto* frame = handle->mount_root()->slnode();
        auto* content = owner->slnode();
        EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(unbounded ? 300 : 200, content->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(40, content->GetBorderBoundHeight());
        float x = !unbounded                       ? 0
                  : std::string(align) == "center" ? -50
                  : std::string(align) == "end"    ? -100
                                                   : 0;
        if (rtl) x = 200 - content->GetBorderBoundWidth() - x;
        EXPECT_FLOAT_EQ(x, LeftFromAncestor(content, frame));
        EXPECT_EQ("wrap-content.png",
                  static_cast<ImageElement*>(owner.get())->src().str());
        page->RemoveNode(handle->mount_root());
      }
    }
  }
}

TEST_P(ModifierElementTest, WrapContentWidthMeasuresRowAndColumnChildren) {
  auto page = CreateComposePage();
  for (bool row : {false, true}) {
    for (bool unbounded : {false, true}) {
      for (bool rtl : {false, true}) {
        for (const auto* align : {"start", "center", "end"}) {
          auto handle = CreateComposeHandle(manager);
          auto width = AppendTestSize({}, 200, 0);
          width.Table()->SetValue("specifiedAxes", lepus::Value(1));
          ASSERT_TRUE(ComposeModifierApplicator::Apply(
                          handle.get(),
                          AppendTestWrapWidth(width, align, unbounded), nullptr)
                          .success);
          auto owner = handle->content_element();
          owner->SetStyle(kPropertyIDDisplay, lepus::Value("linear"));
          owner->SetStyle(kPropertyIDLinearOrientation,
                          lepus::Value(row ? "row" : "column"));
          handle->mount_root()->SetStyle(kPropertyIDDirection,
                                         lepus::Value(rtl ? "rtl" : "ltr"));
          auto first = CreateComposeHandle(manager);
          auto second = CreateComposeHandle(manager);
          for (auto* child : {first.get(), second.get()}) {
            ASSERT_TRUE(
                ComposeModifierApplicator::Apply(
                    child, AppendTestSize({}, row ? 150 : 300, 20), nullptr)
                    .success);
            owner->InsertNode(child->mount_root());
          }
          page->InsertNode(handle->mount_root());
          FlushAndLayout(page);
          auto* frame = handle->mount_root()->slnode();
          auto* content = owner->slnode();
          EXPECT_EQ(starlight::DisplayType::kLinear,
                    content->GetCSSStyle()->GetDisplay(
                        content->GetLayoutConfigs(), content->attr_map()));
          EXPECT_EQ(2u, owner->GetChildCount());
          EXPECT_FLOAT_EQ(200, frame->GetBorderBoundWidth());
          EXPECT_FLOAT_EQ(unbounded ? 300 : 200,
                          content->GetBorderBoundWidth());
          EXPECT_FLOAT_EQ(row ? 20 : 40, frame->GetBorderBoundHeight());
          float x = !unbounded                       ? 0
                    : std::string(align) == "center" ? -50
                    : std::string(align) == "end"    ? -100
                                                     : 0;
          if (rtl) x = 200 - content->GetBorderBoundWidth() - x;
          EXPECT_FLOAT_EQ(x, LeftFromAncestor(content, frame));
          EXPECT_EQ(first->mount_root()->slnode(), content->FirstChild());
          EXPECT_FLOAT_EQ(row ? 0 : 20,
                          second->mount_root()
                              ->slnode()
                              ->GetBorderBoundTopFromParentPaddingBound());
          page->RemoveNode(handle->mount_root());
        }
      }
    }
  }
}

TEST_P(ModifierElementTest, SizeRangePreservesSizePaddingAndRepeatedOrder) {
  auto page = CreateComposePage();
  struct Case {
    lepus::Value source;
    float width;
    float height;
  };
  const std::vector<Case> cases{
      {AppendTestSizeRange(AppendTestSize({}, 100, 30), 1, 0, 0, 50), 100, 30},
      {AppendTestSize(AppendTestSizeRange({}, 1, 0, 0, 50), 100, 30), 50, 30},
      {AppendTestSize(AppendTestSizeRange({}, 1, 80), 20, 30), 80, 30},
      {AppendTestSize(
           AppendTestSizeRange(AppendTestSizeRange({}, 1, 0, 0, 50), 1, 80), 20,
           30),
       50, 30},
      {AppendTestSize(
           AppendTestSizeRange(AppendTestSizeRange({}, 1, 80), 1, 0, 0, 50), 20,
           30),
       80, 30},
      {AppendTestSizeRange(AppendTestSize({}, 20, 100), 2, 0, 0, -1, 50), 20,
       100},
      {AppendTestSize(AppendTestSizeRange({}, 2, 0, 0, -1, 50), 20, 100), 20,
       50},
      {AppendTestSize(AppendTestPadding(AppendTestSizeRange({}, 1, 80)), 20,
                      10),
       80, 26},
      {AppendTestSize(AppendTestSizeRange(AppendTestPadding({}), 1, 80), 20,
                      10),
       96, 26},
      {AppendTestSize(AppendTestSizeRange({}, 3, 80, 40, 100, 70), 200, 100),
       100, 70},
  };
  for (size_t index = 0; index < cases.size(); ++index) {
    SCOPED_TRACE(index);
    auto handle = CreateComposeHandle(manager);
    const auto result = ComposeModifierApplicator::Apply(
        handle.get(), cases[index].source, nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    page->InsertNode(handle->mount_root());
    FlushAndLayout(page);
    EXPECT_FLOAT_EQ(cases[index].width,
                    handle->mount_root()->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(cases[index].height,
                    handle->mount_root()->slnode()->GetBorderBoundHeight());
    for (auto* frame : CollectModifierFrames(handle->mount_root().get(),
                                             handle->content_element().get())) {
      EXPECT_EQ(nullptr, frame->slnode()->GetSLMeasureFunc());
      EXPECT_EQ(nullptr, frame->slnode()->GetSLAlignmentFunc());
    }
    page->RemoveNode(handle->mount_root());
  }
}

TEST_P(ModifierElementTest, SizeRangeRespectsParentConstraints) {
  auto page = CreateComposePage();
  for (bool fixed : {false, true}) {
    for (bool minimum : {false, true}) {
      SCOPED_TRACE(fixed);
      SCOPED_TRACE(minimum);
      auto parent = CreateComposeHandle(manager);
      auto* box = parent->content_element().get();
      ASSERT_TRUE(ComposeModifierApplicator::Apply(
                      parent.get(), AppendTestSize({}, 50, 40), nullptr)
                      .success);
      box->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
      box->SetStyle(kPropertyIDXBoxPropagateMinConstraints,
                    lepus::Value(fixed));
      auto child = CreateComposeHandle(manager);
      auto source = minimum ? AppendTestSizeRange({}, 3, 80, 70)
                            : AppendTestSizeRange({}, 3, 0, 0, 20, 10);
      source = AppendTestSize(source, 100, 100);
      ASSERT_TRUE(ComposeModifierApplicator::Apply(child.get(), source, nullptr)
                      .success);
      box->InsertNode(child->mount_root());
      page->InsertNode(parent->mount_root());
      FlushAndLayout(page);
      EXPECT_FLOAT_EQ(minimum || fixed ? 50 : 20,
                      child->mount_root()->slnode()->GetBorderBoundWidth());
      EXPECT_FLOAT_EQ(minimum || fixed ? 40 : 10,
                      child->mount_root()->slnode()->GetBorderBoundHeight());
      page->RemoveNode(parent->mount_root());
    }
  }
}

TEST_P(ModifierElementTest, SizeRangeRoundsBoundsBeforeCssMeasurement) {
  for (const auto& units :
       std::array<std::array<float, 2>, 4>{{{1, 1}, {2, 1}, {1, 2}, {1, 3}}}) {
    auto page = CreateComposePage(units[0], units[1]);
    auto handle = CreateComposeHandle(manager);
    ASSERT_TRUE(
        ComposeModifierApplicator::Apply(
            handle.get(),
            AppendTestSize(AppendTestSizeRange({}, 3, 1.5, 2.5, 4.25, 5.25), 0,
                           100),
            nullptr)
            .success);
    page->InsertNode(handle->mount_root());
    FlushAndLayout(page);
    const auto round = [&](float value) {
      return std::floor(value * units[0] * units[1] + 0.5f) / units[1];
    };
    EXPECT_FLOAT_EQ(round(1.5),
                    handle->mount_root()->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(round(5.25),
                    handle->mount_root()->slnode()->GetBorderBoundHeight());
    EXPECT_TRUE(RawStyle(handle->content_element().get(), kPropertyIDMinWidth)
                    .IsString());
    EXPECT_TRUE(RawStyle(handle->content_element().get(), kPropertyIDMaxHeight)
                    .IsString());
    page->RemoveNode(handle->mount_root());
  }
}

TEST_P(ModifierElementTest, SizeRangeCloneAndRemovalPreserveOwner) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(
          handle.get(),
          AppendTestSize(AppendTestSizeRange({}, 3, 80, 40, 100, 70), 20, 100),
          nullptr)
          .success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  for (bool resolved : {false, true}) {
    auto clone = TreeResolver::CloneElements(
        handle->mount_root(), handle->mount_root()->style_sheet_manager(),
        resolved, TreeResolver::CloningDepth::kTree);
    page->InsertNode(clone);
    FlushAndLayout(page);
    EXPECT_FLOAT_EQ(80, clone->slnode()->GetBorderBoundWidth());
    EXPECT_FLOAT_EQ(70, clone->slnode()->GetBorderBoundHeight());
    page->RemoveNode(clone);
  }
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestSize({}, 20, 10), nullptr)
                  .success);
  FlushAndLayout(page);
  EXPECT_EQ(owner, handle->mount_root());
  EXPECT_FLOAT_EQ(20, owner->slnode()->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(10, owner->slnode()->GetBorderBoundHeight());
  EXPECT_TRUE(RawStyle(owner.get(), kPropertyIDMinWidth).IsEmpty());
  EXPECT_TRUE(RawStyle(owner.get(), kPropertyIDMaxHeight).IsEmpty());
}

TEST_P(ModifierElementTest, SizeRangeRejectsInvalidIrBeforeChangingTopology) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestSizeRange({}, 1, 20), nullptr)
                  .success);
  auto root = handle->mount_root();
  for (const auto& invalid :
       {AppendTestSizeRange({}, 0), AppendTestSizeRange({}, 4),
        AppendTestSizeRange({}, 1, -2), AppendTestSizeRange({}, 1, 20, 0, 10),
        AppendTestSizeRange({}, 1, 0, 0, -2),
        AppendTestSizeRange({}, 1, std::numeric_limits<double>::infinity())}) {
    EXPECT_FALSE(
        ComposeModifierApplicator::Apply(handle.get(), invalid, nullptr)
            .success);
    EXPECT_EQ(root, handle->mount_root());
  }
}

TEST_P(ModifierElementTest, OffsetPreservesRowSlotAcrossUpdatesAndDirection) {
  for (const auto& units : std::array<std::array<float, 2>, 5>{
           {{1, 1}, {2, 1}, {1, 2}, {1, 1.5}, {1, 3}}}) {
    const float scale = units[0];
    const float physical_scale = units[1];
    auto page = CreateComposePage(scale, physical_scale);
    auto row = CreateComposeHandle(manager)->content_element();
    row->SetStyle(kPropertyIDDisplay, lepus::Value("linear"));
    row->SetStyle(kPropertyIDLinearOrientation, lepus::Value("row"));
    row->SetStyle(kPropertyIDBoxSizing, lepus::Value("border-box"));
    row->SetStyle(kPropertyIDWidth, lepus::Value("200px"));
    row->SetStyle(kPropertyIDHeight, lepus::Value("80px"));
    row->SetStyle(kPropertyIDPaddingLeft, lepus::Value("3px"));
    row->SetStyle(kPropertyIDPaddingRight, lepus::Value("7px"));
    row->SetStyle(kPropertyIDPaddingTop, lepus::Value("5px"));
    row->SetStyle(kPropertyIDPaddingBottom, lepus::Value("9px"));
    row->SetStyle(kPropertyIDBorderLeftWidth, lepus::Value("1px"));
    row->SetStyle(kPropertyIDBorderRightWidth, lepus::Value("2px"));
    row->SetStyle(kPropertyIDBorderTopWidth, lepus::Value("3px"));
    row->SetStyle(kPropertyIDBorderBottomWidth, lepus::Value("4px"));
    auto handle = CreateComposeHandle(manager);
    auto owner = handle->content_element();
    auto tail = CreateComposeHandle(manager);
    ASSERT_TRUE(ComposeModifierApplicator::Apply(
                    handle.get(),
                    AppendTestOffset(AppendTestSize({}, 20, 10), 0, 0), nullptr)
                    .success);
    ASSERT_TRUE(ComposeModifierApplicator::Apply(
                    tail.get(), AppendTestSize({}, 30, 12), nullptr)
                    .success);
    row->InsertNode(handle->mount_root());
    row->InsertNode(tail->mount_root());
    page->InsertNode(row);
    for (bool rtl : {false, true}) {
      row->SetStyle(kPropertyIDDirection, lepus::Value(rtl ? "rtl" : "ltr"));
      std::array<float, 4> original{};
      bool first = true;
      for (const auto& delta :
           std::array<std::array<double, 2>, 8>{{{0, 0},
                                                 {8, -5},
                                                 {-4, 7},
                                                 {8, -5},
                                                 {1.5, -1.5},
                                                 {-1.5, 1.5},
                                                 {1.25, -0.75},
                                                 {123456.5, -123456.5}}}) {
        auto result = ComposeModifierApplicator::Apply(
            handle.get(),
            AppendTestOffset(AppendTestSize({}, 20, 10), delta[0], delta[1]),
            nullptr);
        ASSERT_TRUE(result.success) << result.error_message;
        FlushAndLayout(page);
        auto* frame = handle->mount_root()->slnode();
        auto* child = owner->slnode();
        ASSERT_NE(frame, nullptr);
        ASSERT_NE(child, nullptr);
        EXPECT_EQ(rtl, frame->GetCSSStyle()->IsAnyRtl());
        EXPECT_EQ(nullptr, frame->GetSLAlignmentFunc());
        EXPECT_EQ(nullptr, child->GetSLAlignmentFunc());
        EXPECT_EQ(starlight::PositionType::kRelative,
                  child->GetCSSStyle()->GetPosition());
        EXPECT_FLOAT_EQ(20 * scale, frame->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(10 * scale, frame->GetBorderBoundHeight());
        EXPECT_FLOAT_EQ(20 * scale, child->GetBorderBoundWidth());
        EXPECT_FLOAT_EQ(10 * scale, child->GetBorderBoundHeight());
        // Round in device pixels first, then mirror x for RTL.
        const float x = std::floor(delta[0] * scale * physical_scale + 0.5) /
                        physical_scale;
        const float y = std::floor(delta[1] * scale * physical_scale + 0.5) /
                        physical_scale;
        EXPECT_FLOAT_EQ(
            (rtl ? -1 : 1) * x,
            child->GetBoundLeftFrom(frame, starlight::BoundType::kBorder,
                                    starlight::BoundType::kBorder));
        EXPECT_FLOAT_EQ(
            y, child->GetBoundTopFrom(frame, starlight::BoundType::kBorder,
                                      starlight::BoundType::kBorder));
        const std::array<float, 4> positions{
            frame->GetBorderBoundLeftFromParentPaddingBound(),
            frame->GetBorderBoundTopFromParentPaddingBound(),
            tail->mount_root()
                ->slnode()
                ->GetBorderBoundLeftFromParentPaddingBound(),
            tail->mount_root()
                ->slnode()
                ->GetBorderBoundTopFromParentPaddingBound()};
        if (first) {
          original = positions;
          first = false;
        } else {
          EXPECT_EQ(original, positions);
        }
      }
    }
    page->RemoveNode(row);
  }
}

TEST_P(ModifierElementTest,
       RejectsInvalidOperationIntegersWithoutChangingRoot) {
  auto handle = CreateComposeHandle(manager);
  ASSERT_TRUE(ComposeModifierApplicator::Apply(handle.get(),
                                               AppendTestPadding({}), nullptr)
                  .success);
  auto root = handle->mount_root();
  for (double op :
       {14.5, std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        static_cast<double>(std::numeric_limits<int32_t>::max()) + 1,
        static_cast<double>(std::numeric_limits<int32_t>::min()) - 1}) {
    auto node = lepus::Dictionary::Create();
    node->SetValue("op", lepus::Value(op));
    const auto result = ComposeModifierApplicator::Apply(
        handle.get(), lepus::Value(node), nullptr);
    EXPECT_FALSE(result.success);
    EXPECT_EQ("op should be an integer", result.error_message);
    EXPECT_EQ(root, handle->mount_root());
  }
  auto node = lepus::Dictionary::Create();
  node->SetValue("op", lepus::Value(-0.0));
  const auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(node), nullptr);
  EXPECT_FALSE(result.success);
  EXPECT_NE("op should be an integer", result.error_message);
  EXPECT_EQ(root, handle->mount_root());
}

TEST_P(ModifierElementTest, LayoutValuesKeepCssNumberFormatting) {
  auto handle = CreateComposeHandle(manager);
  struct TestCase {
    double value;
    const char* css_value;
  };
  const TestCase cases[] = {{1.23456789, "1.23457px"},
                            {0.00001, "1e-05px"},
                            {1000000, "1e+06px"},
                            {-0.0, "-0px"}};
  for (const auto& test : cases) {
    auto node = lepus::Dictionary::Create();
    node->SetValue("op", lepus::Value(7));
    for (const auto* side : {"start", "top", "end", "bottom"}) {
      node->SetValue(side, lepus::Value(test.value));
    }
    const auto result = ComposeModifierApplicator::Apply(
        handle.get(), lepus::Value(node), nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(lepus::Value(test.css_value),
              RawStyle(handle->mount_root().get(), kPropertyIDPaddingLeft));
  }
}

TEST_P(ModifierElementTest, OffsetKeepsModifierBoundariesAndClearsOnRemoval) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  const auto owner_id = owner->impl_id();
  auto source = AppendTestStyle({}, kPropertyIDBackgroundColor, "red");
  source = AppendTestOffset(source, 5, 7);
  source = AppendTestStyle(source, kPropertyIDBackgroundColor, "blue");
  source = AppendTestPadding(source);
  source = AppendTestOffset(source, -2, 3);
  source = AppendTestStyle(source, kPropertyIDBackgroundColor, "green");
  source = AppendTestSize(source, 20, 10);
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), source, nullptr).success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  const auto frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(3u, frames.size());
  EXPECT_EQ(lepus::Value("red"),
            RawStyle(frames[0], kPropertyIDBackgroundColor));
  EXPECT_EQ(lepus::Value("blue"),
            RawStyle(frames[1], kPropertyIDBackgroundColor));
  EXPECT_EQ(lepus::Value("green"),
            RawStyle(owner.get(), kPropertyIDBackgroundColor));
  EXPECT_FLOAT_EQ(36, frames[0]->slnode()->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(26, frames[0]->slnode()->GetBorderBoundHeight());
  // Direction changes resolve the existing logical inset without reapplying IR.
  for (bool rtl : {false, true, false}) {
    page->SetStyle(kPropertyIDDirection, lepus::Value(rtl ? "rtl" : "ltr"));
    FlushAndLayout(page);
    float x = 0, y = 0;
    for (auto* current = owner.get(); current != handle->mount_root().get();
         current = current->parent()) {
      EXPECT_EQ(nullptr, current->slnode()->GetSLAlignmentFunc());
      x += current->slnode()->GetBorderBoundLeftFromParentPaddingBound();
      y += current->slnode()->GetBorderBoundTopFromParentPaddingBound();
    }
    // Padding stays 8; the two offsets contribute +(5-2) in LTR and -(5-2) in
    // RTL.
    EXPECT_FLOAT_EQ(rtl ? 5 : 11, x);
    EXPECT_FLOAT_EQ(18, y);
  }
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestSize({}, 20, 10), nullptr)
                  .success);
  FlushAndLayout(page);
  EXPECT_EQ(owner, handle->mount_root());
  EXPECT_EQ(owner_id, owner->impl_id());
  EXPECT_EQ(nullptr, owner->slnode()->GetSLAlignmentFunc());
  EXPECT_TRUE(RawStyle(owner.get(), kPropertyIDInsetInlineStart).IsEmpty());
  EXPECT_TRUE(RawStyle(owner.get(), kPropertyIDTop).IsEmpty());
  EXPECT_FLOAT_EQ(0,
                  owner->slnode()->GetBorderBoundLeftFromParentPaddingBound());
  EXPECT_FLOAT_EQ(0,
                  owner->slnode()->GetBorderBoundTopFromParentPaddingBound());
  EXPECT_FLOAT_EQ(20, owner->slnode()->GetBorderBoundWidth());
  EXPECT_FLOAT_EQ(10, owner->slnode()->GetBorderBoundHeight());
}

TEST_P(ModifierElementTest,
       OffsetRejectsInvalidCoordinatesBeforeChangingTopology) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(), AppendTestOffset({}, 1, -2), nullptr)
                  .success);
  auto root = handle->mount_root();
  for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                         std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::max()}) {
    for (bool invalid_x : {false, true}) {
      auto result = ComposeModifierApplicator::Apply(
          handle.get(),
          AppendTestOffset({}, invalid_x ? invalid : 0,
                           invalid_x ? 0 : invalid),
          nullptr);
      EXPECT_FALSE(result.success);
      EXPECT_EQ(root, handle->mount_root());
    }
  }
  auto missing = lepus::Dictionary::Create();
  missing->SetValue("op", lepus::Value(14));
  missing->SetValue("x", lepus::Value(1));
  EXPECT_FALSE(ComposeModifierApplicator::Apply(handle.get(),
                                                lepus::Value(missing), nullptr)
                   .success);
  missing->SetValue("y", lepus::Value("2"));
  EXPECT_FALSE(ComposeModifierApplicator::Apply(handle.get(),
                                                lepus::Value(missing), nullptr)
                   .success);
  EXPECT_EQ(root, handle->mount_root());
}

TEST_P(ModifierElementTest, OffsetRequiresLayoutInElement) {
  auto handle = CreateComposeHandle(manager);
  auto root = handle->mount_root();
  ASSERT_FALSE(root->EnableLayoutInElementMode());
  EXPECT_FALSE(ComposeModifierApplicator::Apply(
                   handle.get(), AppendTestOffset({}, 1, 2), nullptr)
                   .success);
  EXPECT_EQ(root, handle->mount_root());
}

TEST_P(ModifierElementTest, OffsetClonePreservesRelativeStyles) {
  auto page = CreateComposePage();
  auto handle = CreateComposeHandle(manager);
  ASSERT_TRUE(ComposeModifierApplicator::Apply(
                  handle.get(),
                  AppendTestOffset(AppendTestSize({}, 20, 10), 8, -3), nullptr)
                  .success);
  page->InsertNode(handle->mount_root());
  FlushAndLayout(page);
  for (bool clone_resolved_props : {false, true}) {
    SCOPED_TRACE(clone_resolved_props);
    auto clone = TreeResolver::CloneElements(
        handle->mount_root(), handle->mount_root()->style_sheet_manager(),
        clone_resolved_props, TreeResolver::CloningDepth::kTree);
    auto* child = clone->GetChildAt(0);
    page->InsertNode(clone);
    for (bool rtl : {false, true, false}) {
      page->SetStyle(kPropertyIDDirection, lepus::Value(rtl ? "rtl" : "ltr"));
      FlushAndLayout(page);
      ASSERT_NE(clone->slnode(), nullptr);
      ASSERT_NE(child->slnode(), nullptr);
      EXPECT_EQ(nullptr, clone->slnode()->GetSLAlignmentFunc());
      EXPECT_EQ(nullptr, child->slnode()->GetSLAlignmentFunc());
      EXPECT_FLOAT_EQ(
          rtl ? -8 : 8,
          child->slnode()->GetBorderBoundLeftFromParentPaddingBound());
      EXPECT_FLOAT_EQ(
          -3, child->slnode()->GetBorderBoundTopFromParentPaddingBound());
    }
    page->RemoveNode(clone);
  }
}

TEST_P(ModifierElementTest,
       WeightFillParentDataFollowsAndClearsPhysicalCarrier) {
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  for (bool before_padding : {false, true}) {
    auto source =
        before_padding
            ? AppendTestPadding(AppendTestWeightFill({}, lepus::Value(false)))
            : AppendTestWeightFill(AppendTestPadding({}), lepus::Value(false));
    auto result =
        ComposeModifierApplicator::Apply(handle.get(), source, nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    auto old_root = handle->mount_root();
    ASSERT_NE(old_root, owner);
    ASSERT_NE(old_root->slnode(), nullptr);
    EXPECT_FALSE(old_root->slnode()->GetCSSStyle()->GetComposeWeightFill());
    if (auto* node = owner->slnode())
      EXPECT_TRUE(node->GetCSSStyle()->GetComposeWeightFill());

    result = ComposeModifierApplicator::Apply(
        handle.get(), AppendTestWeightFill({}, lepus::Value(false)), nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(handle->mount_root(), owner);
    EXPECT_TRUE(old_root->slnode()->GetCSSStyle()->GetComposeWeightFill());
    ASSERT_NE(owner->slnode(), nullptr);
    EXPECT_FALSE(owner->slnode()->GetCSSStyle()->GetComposeWeightFill());

    // Invalid IR must leave the current topology and parent data untouched.
    for (const auto& invalid :
         {lepus::Value(), lepus::Value(0), lepus::Value("false"),
          lepus::Value(lepus::Dictionary::Create())}) {
      result = ComposeModifierApplicator::Apply(
          handle.get(), AppendTestWeightFill({}, invalid), nullptr);
      EXPECT_FALSE(result.success);
      EXPECT_EQ(handle->mount_root(), owner);
      EXPECT_FALSE(owner->slnode()->GetCSSStyle()->GetComposeWeightFill());
    }
    result = ComposeModifierApplicator::Apply(handle.get(), {}, nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_TRUE(owner->slnode()->GetCSSStyle()->GetComposeWeightFill());
  }
}

TEST_P(ModifierElementTest, WeightFillLastOperationWinsWithoutCreatingFrames) {
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  for (bool fill : {false, true, false}) {
    const auto source = AppendTestWeightFill(
        AppendTestWeightFill({}, lepus::Value(!fill)), lepus::Value(fill));
    auto result =
        ComposeModifierApplicator::Apply(handle.get(), source, nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(owner, handle->mount_root());
    EXPECT_EQ(0u, owner->GetChildCount());
    EXPECT_EQ(fill, owner->computed_css_style()
                        ->GetConstLayoutComputedStyle()
                        ->GetComposeWeightFill());
  }
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), {}, nullptr).success);
  EXPECT_TRUE(owner->computed_css_style()
                  ->GetConstLayoutComputedStyle()
                  ->GetComposeWeightFill());
}

TEST_P(ModifierElementTest,
       WeightFillRequiresLayoutInElementWithoutChangingTopology) {
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  ASSERT_FALSE(owner->EnableLayoutInElementMode());
  ASSERT_TRUE(ComposeModifierApplicator::Apply(handle.get(),
                                               AppendTestPadding({}), nullptr)
                  .success);
  auto old_root = handle->mount_root();
  ASSERT_NE(old_root, owner);
  const auto result = ComposeModifierApplicator::Apply(
      handle.get(), AppendTestWeightFill({}, lepus::Value(false)), nullptr);
  EXPECT_FALSE(result.success);
  EXPECT_EQ(old_root, handle->mount_root());
  EXPECT_EQ(1u, old_root->GetChildCount());
  EXPECT_EQ(owner.get(), old_root->GetChildAt(0));
  EXPECT_TRUE(old_root->computed_css_style()
                  ->GetConstLayoutComputedStyle()
                  ->GetComposeWeightFill());
}

TEST_P(ModifierElementTest, WeightFillUpdateInvalidatesMeasuredAncestor) {
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  manager->SetLayoutTick(
      [this](const auto& pipeline) { manager->RequestLayout(pipeline); });
  auto page = manager->CreateFiberPage("page", 0);
  page->MarkAsComposeLayout();
  page->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  manager->SetFiberPageElement(page);
  auto parent = CreateComposeHandle(manager)->content_element();
  parent->SetStyle(kPropertyIDDisplay, lepus::Value("linear"));
  parent->SetStyle(kPropertyIDLinearOrientation, lepus::Value("row"));
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  auto make_plan = [](bool fill) {
    auto weight = lepus::Dictionary::Create();
    weight->SetValue("op", lepus::Value(2));
    weight->SetValue(
        "propertyId",
        lepus::Value(static_cast<int32_t>(kPropertyIDLinearWeight)));
    weight->SetValue("value", lepus::Value(1.0));
    return AppendTestWeightFill(lepus::Value(weight), lepus::Value(fill));
  };
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), make_plan(false), nullptr)
          .success);
  parent->InsertNode(owner);
  page->InsertNode(parent);
  page->FlushActionsAsRoot();
  EXPECT_FALSE(owner->slnode()->GetCSSStyle()->GetComposeWeightFill());
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), make_plan(true), nullptr)
          .success);
  owner->slnode()->SetSLComposeMeasureFunc(
      [](void*, const starlight::ComposeConstraints& constraints, bool) {
        return FloatSize(constraints.ConstrainWidth(20),
                         constraints.ConstrainHeight(10));
      });
  owner->slnode()->MarkDirty();
  const starlight::ComposeConstraints range{0, 100, 0, 40};
  auto size =
      parent->slnode()->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_FLOAT_EQ(100, size.width_);
  EXPECT_FLOAT_EQ(100, owner->slnode()->GetBorderBoundWidth());
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), make_plan(false), nullptr)
          .success);
  // Modifier parent data is available immediately in the layout style.
  size = parent->slnode()->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_FLOAT_EQ(20, size.width_);
  EXPECT_FLOAT_EQ(20, owner->slnode()->GetBorderBoundWidth());
  // Rebuilding the CSS style must retain the Modifier's fill flag.
  owner->SetStyle(kPropertyIDOpacity, lepus::Value(0.5));
  page->FlushActionsAsRoot();
  EXPECT_FALSE(owner->slnode()->GetCSSStyle()->GetComposeWeightFill());
  size = parent->slnode()->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_FLOAT_EQ(20, size.width_);
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), make_plan(true), nullptr)
          .success);
  size = parent->slnode()->UpdateMeasureWithComposeConstraints(range, true);
  EXPECT_FLOAT_EQ(100, size.width_);
}

TEST_P(ModifierElementTest, GradientBackgroundUsesAuthoredPaddingBoundary) {
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  constexpr char kGradient[] = "linear-gradient(180deg, red, blue)";

  // Move the same background before, between and after two padding modifiers.
  for (size_t background_position = 0; background_position <= 2;
       ++background_position) {
    SCOPED_TRACE(background_position);
    lepus::Value source;
    for (size_t position = 0; position <= 2; ++position) {
      if (position == background_position) {
        source = AppendTestStyle(source, kPropertyIDBackgroundImage, kGradient);
      }
      if (position < 2) {
        source = AppendTestPadding(source);
      }
    }
    const auto result =
        ComposeModifierApplicator::Apply(handle.get(), source, nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    const auto frames =
        CollectModifierFrames(handle->mount_root().get(), owner.get());
    ASSERT_EQ(frames.size(), 2u);
    for (size_t position = 0; position <= 2; ++position) {
      Element* target = position < 2 ? frames[position] : owner.get();
      EXPECT_EQ(RawStyle(target, kPropertyIDBackgroundImage),
                position == background_position ? lepus::Value(kGradient)
                                                : lepus::Value());
    }
  }
}

TEST_P(ModifierElementTest, ReplacingGradientClearsPreviousBackgroundImage) {
  constexpr char kGradient[] = "linear-gradient(180deg, red, blue)";
  for (const bool background_first : {true, false}) {
    SCOPED_TRACE(background_first);
    auto handle = CreateComposeHandle(manager);
    auto owner = handle->content_element();
    const auto source =
        background_first
            ? AppendTestPadding(AppendTestStyle(
                  lepus::Value(), kPropertyIDBackgroundImage, kGradient))
            : AppendTestStyle(AppendTestPadding(lepus::Value()),
                              kPropertyIDBackgroundImage, kGradient);
    auto result =
        ComposeModifierApplicator::Apply(handle.get(), source, nullptr);
    ASSERT_TRUE(result.success) << result.error_message;

    result = ComposeModifierApplicator::Apply(
        handle.get(),
        AppendTestStyle(lepus::Value(), kPropertyIDBackgroundColor, "#ffffff"),
        nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(handle->mount_root(), owner);
    EXPECT_TRUE(RawStyle(owner.get(), kPropertyIDBackgroundImage).IsEmpty());
    EXPECT_EQ(RawStyle(owner.get(), kPropertyIDBackgroundColor),
              lepus::Value("#ffffff"));
  }
}

TEST_P(ModifierElementTest, UsesDefaultElementBehaviorWithViewTag) {
  auto element = manager->CreateFiberModifierElement();

  EXPECT_TRUE(element->GetTag().IsEqual(kElementViewTag));
  EXPECT_FALSE(element->is_view());
  EXPECT_FALSE(element->is_wrapper());

  const auto& styles = element->GetCurrentRawInlineStyles();
  ASSERT_TRUE(styles.has_value());
  EXPECT_EQ(styles->at(kPropertyIDDisplay), lepus::Value("-x-box"));
  EXPECT_EQ(styles->at(kPropertyIDBoxSizing), lepus::Value("border-box"));
}

TEST_P(ModifierElementTest, ResolvesFiberAndComposeElements) {
  auto element = manager->CreateFiberView();
  lepus::Value element_value{fml::RefPtr<Element>(element)};
  EXPECT_EQ(GetComposeContentOrFiberElementFromValue(element_value), element);
  EXPECT_EQ(GetComposeMountRootOrFiberElementFromValue(element_value), element);

  auto handle = CreateComposeHandle(manager);
  auto content = handle->content_element();
  lepus::Value handle_value{handle};
  EXPECT_EQ(GetComposeContentOrFiberElementFromValue(handle_value), content);
  EXPECT_EQ(GetComposeMountRootOrFiberElementFromValue(handle_value), content);

  auto result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 1.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(result.success) << result.error_message;
  EXPECT_EQ(GetComposeContentOrFiberElementFromValue(handle_value), content);
  EXPECT_EQ(GetComposeMountRootOrFiberElementFromValue(handle_value),
            handle->mount_root());
  EXPECT_NE(handle->mount_root(), content);
}

TEST_P(ModifierElementTest, MaterializesOnlyPaddingAsDetachedFrame) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(1.0));
  padding->SetValue("top", lepus::Value(2.0));
  padding->SetValue("end", lepus::Value(3.0));
  padding->SetValue("bottom", lepus::Value(4.0));

  auto size = lepus::Dictionary::Create();
  size->SetValue("op", lepus::Value(8));
  size->SetValue("specifiedAxes", lepus::Value(3));
  size->SetValue("width", lepus::Value(100.0));
  size->SetValue("height", lepus::Value(50.0));
  size->SetValue("previous", lepus::Value(padding));

  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(size), /*registration_context=*/nullptr);
  ASSERT_TRUE(result.success) << result.error_message;
  auto mount_root = handle->mount_root();
  ASSERT_NE(mount_root, nullptr);
  EXPECT_NE(mount_root.get(), owner.get());
  const auto frames = CollectModifierFrames(mount_root.get(), owner.get());
  ASSERT_EQ(frames.size(), 1u);
  EXPECT_EQ(frames[0]->parent(), nullptr);
  EXPECT_EQ(mount_root.get(), frames[0]);
  ASSERT_EQ(frames[0]->GetChildCount(), 1u);
  EXPECT_EQ(frames[0]->GetChildAt(0), owner.get());
  EXPECT_EQ(owner->parent(), frames[0]);

  const auto& owner_styles = owner->GetCurrentRawInlineStyles();
  ASSERT_TRUE(owner_styles.has_value());
  EXPECT_EQ(owner_styles->at(kPropertyIDWidth), lepus::Value("100px"));
  EXPECT_EQ(owner_styles->at(kPropertyIDHeight), lepus::Value("50px"));
  EXPECT_EQ(owner_styles->at(kPropertyIDBoxSizing), lepus::Value("border-box"));
}

TEST_P(ModifierElementTest, KeepsPhysicalRootsPrivateToEachOwner) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(1.0));
  padding->SetValue("top", lepus::Value(1.0));
  padding->SetValue("end", lepus::Value(1.0));
  padding->SetValue("bottom", lepus::Value(1.0));

  auto first_handle = CreateComposeHandle(manager);
  auto first_owner = first_handle->content_element();
  auto first_result = ComposeModifierApplicator::Apply(
      first_handle.get(), lepus::Value(padding),
      /*registration_context=*/nullptr);
  ASSERT_TRUE(first_result.success) << first_result.error_message;

  auto second_handle = CreateComposeHandle(manager);
  auto second_owner = second_handle->content_element();
  auto second_result = ComposeModifierApplicator::Apply(
      second_handle.get(), lepus::Value(padding),
      /*registration_context=*/nullptr);
  ASSERT_TRUE(second_result.success) << second_result.error_message;

  auto first_root = first_handle->mount_root();
  auto second_root = second_handle->mount_root();
  EXPECT_NE(first_root, second_root);
  EXPECT_EQ(CollectModifierFrames(first_root.get(), first_owner.get()).size(),
            1u);
  EXPECT_EQ(CollectModifierFrames(second_root.get(), second_owner.get()).size(),
            1u);
}

TEST_P(ModifierElementTest, ReparentAttachedFramesThroughTargetOwnedMove) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  owner->MarkCanBeLayoutOnly(false);
  const int32_t owner_id = owner->impl_id();

  auto initial_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 4.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(initial_result.success) << initial_result.error_message;
  page->InsertNode(handle->mount_root());
  page->FlushActionsAsRoot();
  platform_impl_->Flush();

  auto apply_and_flush = [&](size_t frame_count, double value) {
    const auto old_frames =
        CollectModifierFrames(handle->mount_root().get(), owner.get());
    ASSERT_FALSE(old_frames.empty());
    auto* old_source = old_frames.back();
    auto old_source_weak = old_source->WeakFromThis();
    base::Vector<int32_t> old_frame_ids;
    for (auto* frame : old_frames) {
      old_frame_ids.push_back(frame->impl_id());
    }

    platform_impl_->ResetCapturedRemoveSigns();
    auto result = ComposeModifierApplicator::Apply(
        handle.get(), MakePaddingChain(frame_count, value),
        /*registration_context=*/nullptr);
    ASSERT_TRUE(result.success) << result.error_message;

    const auto new_frames =
        CollectModifierFrames(handle->mount_root().get(), owner.get());
    ASSERT_EQ(new_frames.size(), frame_count);
    auto* new_source = new_frames.back();
    EXPECT_EQ(owner->parent(), new_source);
    EXPECT_EQ(owner->render_parent(), old_source);
    EXPECT_TRUE(old_source_weak);

    page->FlushActionsAsRoot();
    EXPECT_EQ(owner->render_parent(), new_source);
    EXPECT_FALSE(old_source_weak);
    platform_impl_->Flush();

    EXPECT_EQ(owner->impl_id(), owner_id);
    EXPECT_EQ(platform_impl_->node_map_.count(owner_id), 1u);
    ASSERT_EQ(platform_impl_->node_map_.count(new_source->impl_id()), 1u);
    EXPECT_EQ(platform_impl_->node_map_.at(owner_id)->parent_->id_,
              new_source->impl_id());
    EXPECT_FALSE(platform_impl_->HasCapturedRemoveSign(owner_id));
    for (const auto old_frame_id : old_frame_ids) {
      EXPECT_EQ(platform_impl_->node_map_.count(old_frame_id), 0u);
    }
  };

  apply_and_flush(1, 8.0);
  apply_and_flush(2, 12.0);
  apply_and_flush(3, 16.0);
  apply_and_flush(2, 20.0);
  apply_and_flush(1, 24.0);
}

TEST_P(ModifierElementTest, CoalescesPendingMovesBeforeSingleFlush) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  owner->MarkCanBeLayoutOnly(false);
  const int32_t owner_id = owner->impl_id();

  auto initial_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 4.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(initial_result.success) << initial_result.error_message;
  page->InsertNode(handle->mount_root());
  page->FlushActionsAsRoot();
  platform_impl_->Flush();

  const auto initial_frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(initial_frames.size(), 1u);
  auto* initial_source = initial_frames.back();
  auto initial_source_weak = initial_source->WeakFromThis();
  const int32_t initial_frame_id = initial_source->impl_id();

  auto intermediate_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(2, 8.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(intermediate_result.success) << intermediate_result.error_message;
  const auto intermediate_frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(intermediate_frames.size(), 2u);
  auto* intermediate_source = intermediate_frames.back();
  auto intermediate_source_weak = intermediate_source->WeakFromThis();
  base::Vector<int32_t> intermediate_frame_ids;
  for (auto* frame : intermediate_frames) {
    intermediate_frame_ids.push_back(frame->impl_id());
  }
  EXPECT_TRUE(initial_source_weak);

  auto final_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 12.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(final_result.success) << final_result.error_message;
  const auto final_frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(final_frames.size(), 1u);
  auto* final_source = final_frames.back();

  EXPECT_EQ(owner->parent(), final_source);
  EXPECT_EQ(owner->render_parent(), initial_source);
  EXPECT_TRUE(initial_source_weak);

  platform_impl_->ResetCapturedRemoveSigns();
  page->FlushActionsAsRoot();
  EXPECT_EQ(owner->render_parent(), final_source);
  EXPECT_FALSE(initial_source_weak);
  EXPECT_FALSE(intermediate_source_weak);
  platform_impl_->Flush();

  EXPECT_EQ(owner->impl_id(), owner_id);
  EXPECT_EQ(platform_impl_->node_map_.count(owner_id), 1u);
  ASSERT_EQ(platform_impl_->node_map_.count(final_source->impl_id()), 1u);
  EXPECT_EQ(platform_impl_->node_map_.at(owner_id)->parent_->id_,
            final_source->impl_id());
  EXPECT_EQ(platform_impl_->node_map_.count(initial_frame_id), 0u);
  for (const auto intermediate_frame_id : intermediate_frame_ids) {
    EXPECT_EQ(platform_impl_->node_map_.count(intermediate_frame_id), 0u);
  }
  EXPECT_FALSE(platform_impl_->HasCapturedRemoveSign(owner_id));
}

TEST_P(ModifierElementTest, AcceptsComposeModifierSingletonAsChainEnd) {
  auto compose_modifier = lepus::Dictionary::Create();
  auto size = lepus::Dictionary::Create();
  size->SetValue("op", lepus::Value(8));
  size->SetValue("specifiedAxes", lepus::Value(1));
  size->SetValue("width", lepus::Value(40.0));
  size->SetValue("height", lepus::Value(0.0));
  size->SetValue("previous", lepus::Value(compose_modifier));

  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(size), /*registration_context=*/nullptr);

  ASSERT_TRUE(result.success) << result.error_message;
  EXPECT_EQ(handle->mount_root(), owner);
  ASSERT_TRUE(owner->GetCurrentRawInlineStyles().has_value());
  EXPECT_EQ(owner->GetCurrentRawInlineStyles()->at(kPropertyIDWidth),
            lepus::Value("40px"));
}

TEST_P(ModifierElementTest,
       BuiltinAndCustomBindingsUseNormalModifierAndChildOperations) {
  auto runtime_ = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_NE(runtime_, nullptr);
  runtime_->Initialize();
  runtime_->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto Eval = [&](const std::string& script) {
    lepus::Value value;
    EXPECT_TRUE(runtime::MTSRuntime::ToQuickContext(runtime_.get())
                    ->EvalBuf(script.c_str(), script.size(), value,
                              "compose_custom_test.js"));
    return value;
  };
  auto page = manager->CreateFiberPage("page", 11);
  Renderer::RegisterBuiltin(runtime_.get(), ArchOption::FIBER_ARCH);
  runtime_->SetGlobalData("customPage", lepus::Value(page));
  runtime_->SetGlobalData("customPageId", lepus::Value(page->impl_id()));
  // Renderer validation failures return an empty lepus::Value (JS null).
  EXPECT_TRUE(Eval("__CreateCompose(customPageId, 0) === null").Bool());
  EXPECT_TRUE(Eval("__CreateCompose(customPageId, 0, '') === null").Bool());
  EXPECT_TRUE(Eval("__CreateCompose(customPageId, 0, null) === null").Bool());
  EXPECT_TRUE(Eval("__CreateCompose(customPageId, 0, 42) === null").Bool());
  EXPECT_TRUE(Eval("__CreateCompose(customPageId, 10) === null").Bool());
  EXPECT_TRUE(
      Eval("__CreateCompose(customPageId, 0.5, 'sample-widget') === null")
          .Bool());
  const struct {
    ComposeElementKind kind;
    const char* tag;
  } cases[] = {{ComposeElementKind::kSvg, "svg"},
               {ComposeElementKind::kOverlay, "overlay"},
               {ComposeElementKind::kAnimaX, "animax-view"},
               {ComposeElementKind::kInput, "input"},
               {ComposeElementKind::kTextarea, "textarea"},
               {ComposeElementKind::kCustom, "sample-widget"},
               {ComposeElementKind::kCustom, "svg"},
               {ComposeElementKind::kCustom, "overlay"}};
  for (const auto& test_case : cases) {
    runtime_->SetGlobalData("customKind",
                            lepus::Value(static_cast<int>(test_case.kind)));
    runtime_->SetGlobalData("customTag", lepus::Value(test_case.tag));
    EXPECT_TRUE(Eval(R"JS(
    globalThis.custom = customKind !== 0
      ? __CreateCompose(customPageId, customKind)
      : __CreateCompose(customPageId, customKind, customTag);
    globalThis.customChild = __CreateCompose(customPageId, 1);
    __SetComposeModifier(custom, {op:6,eventName:'ready',eventType:'bindEvent',
      callback:function(){},previous:{op:4,name:'enabled',value:true}});
    __InsertElementAt(custom, customChild, 0);
    __InsertElementAt(customPage, custom, 0);
    true;
  )JS")
                    .Bool());
    auto custom_value = Eval("custom");
    auto custom = GetComposeMountRootOrFiberElementFromValue(custom_value);
    auto child_value = Eval("customChild");
    auto child = GetComposeMountRootOrFiberElementFromValue(child_value);
    ASSERT_NE(custom, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_TRUE(custom->GetTag().IsEqual(test_case.tag));
    EXPECT_EQ(custom->parent(), page.get());
    EXPECT_EQ(child->parent(), custom.get());
    EXPECT_TRUE(custom->HasEventListener("ready"));
    const auto& attributes = custom->data_model()->attributes();
    auto enabled = attributes.find(base::String("enabled"));
    ASSERT_NE(enabled, attributes.end());
    EXPECT_TRUE(enabled->second.Bool());
    EXPECT_TRUE(Eval(R"JS(
    __SetComposeModifier(custom, {op:4,name:'enabled',value:false});
    __RemoveElementsAt(customPage, 0, 1);
    true;
  )JS")
                    .Bool());
    EXPECT_FALSE(custom->HasEventListener("ready"));
    enabled = attributes.find(base::String("enabled"));
    ASSERT_NE(enabled, attributes.end());
    EXPECT_FALSE(enabled->second.Bool());
    EXPECT_EQ(custom->parent(), nullptr);
  }
}

TEST_P(ModifierElementTest, SynchronousListJumpBindingValidatesInputAndLayout) {
  auto runtime_ = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  runtime_->Initialize();
  runtime_->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto Eval = [&](const std::string& script) {
    lepus::Value value;
    EXPECT_TRUE(runtime::MTSRuntime::ToQuickContext(runtime_.get())
                    ->EvalBuf(script.c_str(), script.size(), value,
                              "sync_list_test.js"));
    return value;
  };
  auto page = manager->CreateFiberPage("page", 11);
  Renderer::RegisterBuiltin(runtime_.get(), ArchOption::FIBER_ARCH);
  runtime_->SetGlobalData("scrollPageId", lepus::Value(page->impl_id()));
  EXPECT_TRUE(Eval(R"JS(
    globalThis.scrollList = __CreateCompose(scrollPageId, 4);
    globalThis.scrollView = __CreateCompose(scrollPageId, 1);
    typeof __InvokeMethod === 'function';
  )JS")
                  .Bool());
  for (const auto& args :
       {"scrollView,'scrollToPosition',{index:0,offset:0}",
        "scrollList,'unknown',{}",
        "null,'scrollToPosition',{}",
        "{},'scrollToPosition',{}",
        "[],'scrollToPosition',{}",
        "42,'scrollToPosition',{}",
        "scrollList,42,{}",
        "scrollView,'scrollBy',{delta:1}",
        "scrollList,'scrollBy',{}",
        "scrollList,'scrollBy',{delta:'1'}",
        "scrollList,'scrollBy',{delta:NaN}",
        "scrollList,'scrollBy',{delta:Infinity}",
        "scrollList,'scrollBy',{delta:1e100}",
        "scrollList,'scrollBy',{delta:1}",
        "scrollList,'scrollToPosition',null",
        "scrollList,'scrollToPosition',{}",
        "scrollList,'scrollToPosition',{index:'0',offset:0}",
        "scrollList,'scrollToPosition',{index:0,offset:'0'}",
        "scrollList,'scrollToPosition',{index:-1,offset:0}",
        "scrollList,'scrollToPosition',{index:0.5,offset:0}",
        "scrollList,'scrollToPosition',{index:Infinity,offset:0}",
        "scrollList,'scrollToPosition',{index:2147483648,offset:0}",
        "scrollList,'scrollToPosition',{index:0,offset:NaN}",
        "scrollList,'scrollToPosition',{index:0,offset:1e100}",
        "scrollList,'scrollToPosition',{index:0,offset:0}"}) {
    SCOPED_TRACE(args);
    base::ErrorStorage::GetInstance().Reset();
    EXPECT_TRUE(
        Eval(std::string("__InvokeMethod(") + args + ") == null").Bool());
    // Every rejection, including an unlaid-out list, is reported.
    EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  }
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ModifierElementTest, SupportsEveryComposeOwnerElementKind) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(2.0));
  padding->SetValue("top", lepus::Value(2.0));
  padding->SetValue("end", lepus::Value(2.0));
  padding->SetValue("bottom", lepus::Value(2.0));

  const ComposeElementKind kinds[] = {
      ComposeElementKind::kView,     ComposeElementKind::kText,
      ComposeElementKind::kImage,    ComposeElementKind::kList,
      ComposeElementKind::kSvg,      ComposeElementKind::kOverlay,
      ComposeElementKind::kAnimaX,   ComposeElementKind::kInput,
      ComposeElementKind::kTextarea, ComposeElementKind::kCustom};
  for (const auto kind : kinds) {
    auto handle =
        CreateComposeHandle(manager, kind, base::String("sample-widget"));
    auto owner = handle->content_element();
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(handle->mount_root(), owner);
    switch (kind) {
      case ComposeElementKind::kView:
        EXPECT_TRUE(owner->is_view());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementViewTag));
        break;
      case ComposeElementKind::kText:
        EXPECT_TRUE(owner->is_text());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementTextTag));
        break;
      case ComposeElementKind::kImage:
        EXPECT_TRUE(owner->is_image());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementImageTag));
        break;
      case ComposeElementKind::kSvg:
        EXPECT_TRUE(owner->GetTag().IsEqual("svg"));
        break;
      case ComposeElementKind::kOverlay:
        EXPECT_TRUE(owner->GetTag().IsEqual("overlay"));
        break;
      case ComposeElementKind::kAnimaX:
        EXPECT_TRUE(owner->GetTag().IsEqual("animax-view"));
        break;
      case ComposeElementKind::kInput:
        EXPECT_TRUE(owner->GetTag().IsEqual("input"));
        break;
      case ComposeElementKind::kTextarea:
        EXPECT_TRUE(owner->GetTag().IsEqual("textarea"));
        break;
      case ComposeElementKind::kCustom:
        EXPECT_TRUE(owner->GetTag().IsEqual("sample-widget"));
        break;
      case ComposeElementKind::kList:
        EXPECT_TRUE(owner->is_list());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementListTag));
        break;
    }
    auto result = ComposeModifierApplicator::Apply(
        handle.get(), lepus::Value(padding), /*registration_context=*/nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    auto root = handle->mount_root();
    EXPECT_NE(root.get(), owner.get());
    EXPECT_EQ(CollectModifierFrames(root.get(), owner.get()).size(), 1u);
  }
}

TEST_P(ModifierElementTest,
       HandleRetainsDetachedChainAndBecomesStaleOnTeardown) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(3.0));
  padding->SetValue("top", lepus::Value(3.0));
  padding->SetValue("end", lepus::Value(3.0));
  padding->SetValue("bottom", lepus::Value(3.0));

  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(padding), /*registration_context=*/nullptr);
  ASSERT_TRUE(result.success) << result.error_message;
  auto root = handle->mount_root();
  auto owner_weak = owner->WeakFromThis();
  auto root_weak = root->WeakFromThis();

  owner = nullptr;
  root = nullptr;
  // The handle, not ElementManager, retains the detached physical chain.
  EXPECT_TRUE(owner_weak);
  EXPECT_TRUE(root_weak);

  manager->node_manager()->WillDestroy();
  std::string error;
  EXPECT_FALSE(
      ComposeModifierApplicator::ValidateTopology(handle.get(), &error));
  EXPECT_TRUE(owner_weak);
  EXPECT_TRUE(root_weak);
  handle = nullptr;
  EXPECT_FALSE(owner_weak);
  EXPECT_FALSE(root_weak);
}

TEST_P(ModifierElementTest, GetChildrenResolvesComposeContent) {
  auto runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_NE(runtime, nullptr);
  runtime->Initialize();
  runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  Renderer::RegisterBuiltin(runtime.get(), ArchOption::FIBER_ARCH);
  auto eval = [&](const std::string& script) {
    lepus::Value value;
    EXPECT_TRUE(runtime::MTSRuntime::ToQuickContext(runtime.get())
                    ->EvalBuf(script.c_str(), script.size(), value,
                              "element_handles.js"));
    return value;
  };
  for (const bool use_handles : {false, true}) {
    SCOPED_TRACE(use_handles);
    struct Node {
      lepus::Value value;
      fml::RefPtr<Element> content;
      fml::RefPtr<Element> root;
    };
    auto create_node = [&]() -> Node {
      if (!use_handles) {
        auto element = manager->CreateFiberElement("sample-widget");
        return {lepus::Value(element), element, element};
      }
      auto handle = CreateComposeHandle(manager, ComposeElementKind::kCustom,
                                        base::String("sample-widget"));
      const auto result = ComposeModifierApplicator::Apply(
          handle.get(), MakePaddingChain(2, 4.0), nullptr);
      EXPECT_TRUE(result.success) << result.error_message;
      EXPECT_NE(handle->content_element(), handle->mount_root());
      return {lepus::Value(handle), handle->content_element(),
              handle->mount_root()};
    };
    const auto parent = create_node();
    const auto first = create_node();
    const auto second = create_node();
    auto tail = manager->CreateFiberView();
    runtime->SetGlobalData("parent", parent.value);
    parent.content->InsertNode(second.root);
    parent.content->InsertNode(first.root);
    parent.content->InsertNode(tail);
    auto children = eval("__GetChildren(parent)");
    ASSERT_TRUE(children.IsArrayOrJSArray());
    ASSERT_EQ(children.GetLength(), 3u);
    EXPECT_EQ(children.GetProperty(uint32_t{0}).RefCounted(), second.root);
    EXPECT_EQ(children.GetProperty(uint32_t{1}).RefCounted(), first.root);
    EXPECT_EQ(children.GetProperty(uint32_t{2}).RefCounted(), tail);

    parent.content->RemoveNode(second.root);
    parent.content->RemoveNode(first.root);
    parent.content->RemoveNode(tail);
    children = eval("__GetChildren(parent)");
    ASSERT_TRUE(children.IsArrayOrJSArray());
    EXPECT_EQ(children.GetLength(), 0u);
  }
}

INSTANTIATE_TEST_SUITE_P(ModifierElementTestModule, ModifierElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

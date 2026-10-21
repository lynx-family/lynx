// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_element.h"

#include <string>
#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/dom/fiber/element_utils.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

fml::RefPtr<ComposeElementHandle> CreateComposeHandle(
    ElementManager* manager,
    ComposeElementKind kind = ComposeElementKind::kView,
    const base::String& tag = base::String()) {
  return fml::AdoptRef<ComposeElementHandle>(
      new ComposeElementHandle(manager, kind, tag));
}

std::vector<ModifierElement*> CollectModifierFrames(Element* root,
                                                    Element* owner) {
  std::vector<ModifierElement*> frames;
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

}  // namespace

class ModifierElementTest : public FiberElementTest {
 public:
  void InitializeRuntime() {
    runtime_ = runtime::MTSRuntime::CreateContext(
        runtime::ContextType::LepusNGContextType);
    ASSERT_NE(runtime_, nullptr);
    runtime_->Initialize();
    runtime_->SetGlobalData(
        BASE_STATIC_STRING(tasm::kTemplateAssembler),
        lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
    Renderer::RegisterBuiltin(runtime_.get(), ArchOption::FIBER_ARCH);
  }

  lepus::Value Eval(const std::string& script) {
    lepus::Value value;
    EXPECT_TRUE(runtime::MTSRuntime::ToQuickContext(runtime_.get())
                    ->EvalBuf(script.c_str(), script.size(), value,
                              "compose_size_range_test.js"));
    return value;
  }

  std::shared_ptr<runtime::MTSRuntime> runtime_;
};

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

TEST_P(ModifierElementTest, SizeRangePreservesOrderAndIncomingBounds) {
  InitializeRuntime();
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  auto handle = CreateComposeHandle(manager);
  page->InsertNode(handle->mount_root());
  runtime_->SetGlobalData("rangeOwner", lepus::Value(handle));
  Eval(R"JS(
    function range(previous, min, max) {
      return {op:11, specifiedAxes:1, minWidth:min, minHeight:0,
              maxWidth:max, maxHeight:-1, previous:previous};
    }
    function size(previous, width) {
      return {op:8, specifiedAxes:3, width:width, height:20, previous:previous};
    }
  )JS");
  struct Case {
    const char* source;
    float available_width;
    float expected;
  };
  const Case cases[] = {
      {"size(range(null, 0, 100), 200)", 300, 100},
      {"range(size(null, 200), 0, 100)", 300, 200},
      {"size(range(null, 60, 100), 20)", 300, 60},
      {"size(range(null, 60, 100), 20)", 40, 40},
      {"size(range(range(null, 0, 100), 0, 200), 300)", 400, 100},
      {"size(range(range(null, 100, 300), 0, 50), 20)", 400, 100},
      {"size(range(range(null, 0, 50), 100, 300), 20)", 400, 50},
      {"size(range(null, 100, 50), 20)", 300, 50},
      {"size(range(null, 60, -1), 20)", 300, 60},
      {"size(range(null, -1, 50), 20)", 300, 50},
      {"size(range(null, -1, -1), 20)", 300, 20},
  };
  for (const auto& test : cases) {
    SCOPED_TRACE(test.source);
    Eval(std::string("__SetComposeModifier(rangeOwner, ") + test.source + ")");
    page->FlushActionsAsRoot();
    manager->OnUpdateViewport(test.available_width, SLMeasureModeDefinite, 200,
                              SLMeasureModeDefinite, false);
    manager->RequestLayout(std::make_shared<PipelineOptions>());
    EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundWidth(),
                    test.expected);
    EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundHeight(), 20);
  }
}

TEST_P(ModifierElementTest, SizeRangeCapsFillAndPaddingOnBothAxes) {
  InitializeRuntime();
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  auto handle = CreateComposeHandle(manager);
  page->InsertNode(handle->mount_root());
  runtime_->SetGlobalData("rangeOwner", lepus::Value(handle));
  Eval(R"JS(
    __SetComposeModifier(rangeOwner, {
      op:9, specifiedAxes:3, fraction:1, previous:{
        op:7, start:4, end:4, top:4, bottom:4, previous:{
          op:11, specifiedAxes:3, minWidth:20, minHeight:30,
          maxWidth:100, maxHeight:80
        }
      }
    });
  )JS");
  page->FlushActionsAsRoot();
  manager->OnUpdateViewport(300, SLMeasureModeDefinite, 200,
                            SLMeasureModeDefinite, false);
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundWidth(), 100);
  EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundHeight(), 80);
  EXPECT_FLOAT_EQ(handle->content_element()->slnode()->GetBorderBoundWidth(),
                  92);
  EXPECT_FLOAT_EQ(handle->content_element()->slnode()->GetBorderBoundHeight(),
                  72);

  // A bounded range supplies the percentage base for fractional fill as well.
  Eval(R"JS(__SetComposeModifier(rangeOwner, {
      op:9, specifiedAxes:3, fraction:0.5, previous:{
        op:11, specifiedAxes:3, minWidth:20, minHeight:30,
        maxWidth:100, maxHeight:80
      }
    });)JS");
  page->FlushActionsAsRoot();
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundWidth(), 50);
  EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundHeight(), 40);

  // Removing a range removes its measured boundary and its old constraints.
  Eval(R"JS(__SetComposeModifier(rangeOwner,
      {op:8,specifiedAxes:3,width:20,height:10});)JS");
  page->FlushActionsAsRoot();
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_EQ(handle->mount_root(), handle->content_element());
  EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundWidth(), 20);
  EXPECT_FLOAT_EQ(handle->mount_root()->slnode()->GetBorderBoundHeight(), 10);
}

TEST_P(ModifierElementTest, SizeRangeFlushesUpdatesAndClearsCustomLayout) {
  class TestModifierElement : public ModifierElement {
   public:
    using Element::PerformElementContainerCreateOrUpdate;
    using ModifierElement::ModifierElement;
  };
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  auto frame = fml::AdoptRef(new TestModifierElement(manager));
  auto child = manager->CreateFiberView();
  child->SetStyle(kPropertyIDWidth, lepus::Value("200px"));
  child->SetStyle(kPropertyIDHeight, lepus::Value("20px"));
  frame->InsertNode(child);
  page->InsertNode(frame);

  frame->SetSizeRange({0, 0, 100, -1});
  EXPECT_TRUE(frame->dirty() & Element::kDirtyCustomLayout);
  EXPECT_TRUE(!frame->slnode() || !frame->slnode()->GetSLMeasureFunc());
  page->FlushActionsAsRoot();
  ASSERT_NE(frame->slnode()->GetSLMeasureFunc(), nullptr);
  ASSERT_NE(frame->slnode()->GetSLAlignmentFunc(), nullptr);
  EXPECT_FALSE(frame->dirty() & Element::kDirtyCustomLayout);
  manager->OnUpdateViewport(300, SLMeasureModeDefinite, 200,
                            SLMeasureModeDefinite, false);
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_FLOAT_EQ(frame->slnode()->GetBorderBoundWidth(), 100);
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundWidth(), 100);

  // The callbacks stay the same, but changed parameters must invalidate layout.
  const auto measure = frame->slnode()->GetSLMeasureFunc();
  frame->SetSizeRange({0, 0, 150, -1});
  EXPECT_EQ(frame->slnode()->GetSLMeasureFunc(), measure);
  // A direct create/update call must consume a configuration-only change even
  // when there are no style/attribute updates and PrepareForCreateOrUpdate is
  // not called (for example, during parallel task reduction).
  EXPECT_EQ(frame->dirty(), Element::kDirtyCustomLayout);
  frame->PerformElementContainerCreateOrUpdate(false, true);
  EXPECT_FALSE(frame->dirty() & Element::kDirtyCustomLayout);
  EXPECT_TRUE(frame->slnode()->IsDirty());
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_FLOAT_EQ(frame->slnode()->GetBorderBoundWidth(), 150);
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundWidth(), 150);

  // Replacing only the layout callback takes effect on the next Flush.
  const auto layout = frame->slnode()->GetSLAlignmentFunc();
  frame->SetCustomLayoutFunctions({measure,
                                   [](void* context) {
                                     static_cast<Element*>(context)
                                         ->GetChildAt(0)
                                         ->slnode()
                                         ->AlignmentByPlatform(3, 7);
                                   },
                                   [](void*, bool) { return false; }});
  EXPECT_EQ(frame->slnode()->GetSLAlignmentFunc(), layout);
  page->FlushActionsAsRoot();
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundLeftFromParentPaddingBound(),
                  7);
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundTopFromParentPaddingBound(),
                  3);

  frame->SetCustomLayoutFunctions({});
  EXPECT_EQ(frame->slnode()->GetSLMeasureFunc(), measure);
  frame->PerformElementContainerCreateOrUpdate(false, true);
  EXPECT_EQ(frame->slnode()->GetSLMeasureFunc(), nullptr);
  EXPECT_EQ(frame->slnode()->GetSLAlignmentFunc(), nullptr);
  EXPECT_FALSE(frame->dirty() & Element::kDirtyCustomLayout);
  manager->RequestLayout(std::make_shared<PipelineOptions>());
  EXPECT_FLOAT_EQ(frame->slnode()->GetBorderBoundWidth(), 200);
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundWidth(), 200);
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundLeftFromParentPaddingBound(),
                  0);
  EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundTopFromParentPaddingBound(),
                  0);
}

TEST_P(ModifierElementTest, ClonedSizeRangeBindsItsOwnLayoutDuringFlush) {
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  auto source = fml::AdoptRef(new ModifierElement(manager));
  source->SetSizeRange({0, 0, 100, -1});
  page->InsertNode(source);
  manager->OnUpdateViewport(300, SLMeasureModeDefinite, 200,
                            SLMeasureModeDefinite, false);

  for (bool flush_source : {false, true}) {
    if (flush_source) page->FlushActionsAsRoot();
    for (bool clone_resolved_props : {false, true}) {
      SCOPED_TRACE(flush_source);
      SCOPED_TRACE(clone_resolved_props);
      auto clone = source->CloneElement(clone_resolved_props);
      EXPECT_EQ(clone->element_manager(), nullptr);
      EXPECT_EQ(clone->slnode(), nullptr);
      EXPECT_TRUE(clone->dirty() & Element::kDirtyCustomLayout);
      clone->AttachToElementManager(
          manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);
      auto child = manager->CreateFiberView();
      child->SetStyle(kPropertyIDWidth, lepus::Value("200px"));
      child->SetStyle(kPropertyIDHeight, lepus::Value("20px"));
      clone->InsertNode(child);
      page->InsertNode(clone);
      page->FlushActionsAsRoot();
      EXPECT_EQ(clone->slnode()->GetContext(), clone.get());
      manager->RequestLayout(std::make_shared<PipelineOptions>());
      EXPECT_FLOAT_EQ(clone->slnode()->GetBorderBoundWidth(), 100);
      EXPECT_FLOAT_EQ(child->slnode()->GetBorderBoundWidth(), 100);

      // Updating the clone must not overwrite the source's parameters.
      static_cast<ModifierElement*>(clone.get())->SetSizeRange({0, 0, 80, -1});
      page->FlushActionsAsRoot();
      manager->RequestLayout(std::make_shared<PipelineOptions>());
      EXPECT_FLOAT_EQ(clone->slnode()->GetBorderBoundWidth(), 80);
      EXPECT_EQ(source->slnode()->GetContext(), source.get());
      page->RemoveNode(clone);
    }
  }
}

TEST_P(ModifierElementTest, SizeRangeKeepsOtherFrameOverflowDefaults) {
  InitializeRuntime();
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto page = manager->CreateFiberPage("page", 11);
  const bool default_visible = manager->GetDefaultOverflowVisible();
  for (const bool explicit_clip : {false, true}) {
    SCOPED_TRACE(explicit_clip);
    auto handle = CreateComposeHandle(manager);
    auto prefix = AppendTestPadding(lepus::Value());
    if (explicit_clip) {
      prefix = AppendTestStyle(prefix, kPropertyIDOverflow, "hidden");
    }
    runtime_->SetGlobalData("rangePrefix", prefix);
    const auto range = Eval(R"JS(({op:11,specifiedAxes:1,minWidth:0,minHeight:0,
        maxWidth:100,maxHeight:-1,previous:rangePrefix}))JS");
    const auto result = ComposeModifierApplicator::Apply(
        handle.get(), AppendTestPadding(range), runtime_.get());
    ASSERT_TRUE(result.success) << result.error_message;
    page->InsertNode(handle->mount_root());
    page->FlushActionsAsRoot();
    const auto frames = CollectModifierFrames(handle->mount_root().get(),
                                              handle->content_element().get());
    ASSERT_EQ(frames.size(), 3u);
    // A range changes only its own default; padding keeps the page default.
    EXPECT_EQ(frames[0]->computed_css_style()->IsOverflowXY(), default_visible);
    EXPECT_EQ(frames[2]->computed_css_style()->IsOverflowXY(), default_visible);
    EXPECT_EQ(frames[1]->computed_css_style()->IsOverflowXY(), !explicit_clip);
    if (explicit_clip) {
      EXPECT_TRUE(frames[1]->computed_css_style()->IsOverflowHidden());
    }
  }
}

TEST_P(ModifierElementTest, InvalidSizeRangeLeavesMountedTreeUnchanged) {
  InitializeRuntime();
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  tasm->SetPageOptions(options);
  auto handle = CreateComposeHandle(manager);
  const auto valid = Eval(R"JS(({op:11,specifiedAxes:1,minWidth:0,minHeight:0,
                                maxWidth:100,maxHeight:-1}))JS");
  ASSERT_TRUE(
      ComposeModifierApplicator::Apply(handle.get(), valid, runtime_.get())
          .success);
  const auto root = handle->mount_root();
  const char* invalid[] = {
      "({op:11,specifiedAxes:0,minWidth:0,minHeight:0,maxWidth:100,maxHeight:-"
      "1})",
      "({op:11,specifiedAxes:4,minWidth:0,minHeight:0,maxWidth:100,maxHeight:-"
      "1})",
      "({op:11,specifiedAxes:1,minWidth:-0.5,minHeight:0,maxWidth:100,"
      "maxHeight:-1})",
      "({op:11,specifiedAxes:1,minWidth:0,minHeight:0,maxWidth:-0.5,maxHeight:-"
      "1})",
      "({op:11,specifiedAxes:1,minWidth:0,minHeight:1,maxWidth:100,maxHeight:-"
      "1})",
      "({op:11,specifiedAxes:1,minWidth:0,minHeight:0,maxWidth:Infinity,"
      "maxHeight:-1})",
  };
  for (const auto* source : invalid) {
    SCOPED_TRACE(source);
    EXPECT_FALSE(ComposeModifierApplicator::Apply(handle.get(), Eval(source),
                                                  runtime_.get())
                     .success);
    EXPECT_EQ(handle->mount_root(), root);
    EXPECT_EQ(root->GetChildAt(0), handle->content_element().get());
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
    std::vector<int32_t> old_frame_ids;
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
  std::vector<int32_t> intermediate_frame_ids;
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
  EXPECT_TRUE(
      Eval("__CreateCompose(customPageId, 0.5, 'sample-widget') === null")
          .Bool());
  const struct {
    ComposeElementKind kind;
    const char* tag;
  } cases[] = {{ComposeElementKind::kSvg, "svg"},
               {ComposeElementKind::kOverlay, "overlay"},
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

TEST_P(ModifierElementTest, SupportsEveryComposeOwnerElementKind) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(2.0));
  padding->SetValue("top", lepus::Value(2.0));
  padding->SetValue("end", lepus::Value(2.0));
  padding->SetValue("bottom", lepus::Value(2.0));

  const ComposeElementKind kinds[] = {
      ComposeElementKind::kView,  ComposeElementKind::kText,
      ComposeElementKind::kImage, ComposeElementKind::kList,
      ComposeElementKind::kSvg,   ComposeElementKind::kOverlay,
      ComposeElementKind::kCustom};
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

INSTANTIATE_TEST_SUITE_P(ModifierElementTestModule, ModifierElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

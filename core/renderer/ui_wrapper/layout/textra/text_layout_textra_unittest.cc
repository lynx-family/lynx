// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"

#include <memory>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_api.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

class DetachedTextElement : public TextElement {
 public:
  DetachedTextElement() : TextElement(nullptr, "text") {
    platform_css_style_ =
        std::make_unique<starlight::ComputedCSSStyle>(1.f, 1.);
  }
};

struct BackendState {
  bool fail_builder{false};
  bool fail_paragraph{false};
  int paragraph_token{0};
  int measured_count{0};
  int destroyed_paragraph_count{0};
  int live_builder_count{0};
  int created_page_count{0};
  int destroyed_page_count{0};
  int live_page_count{0};
  text::Page* last_page{nullptr};
  text::MeasureResult result{0, 0, 0};
  text::MeasureParams params{};
};

class FakeParagraphBuilder : public text::ParagraphBuilder {
 public:
  explicit FakeParagraphBuilder(BackendState& state) : state_(state) {
    ++state_.live_builder_count;
  }
  ~FakeParagraphBuilder() override { --state_.live_builder_count; }

  void SetParagraphListener(text::ParagraphListener*) override {}
  void SetParagraphStyle(TextPropertyKeyID, void*, size_t) override {}
  void PushTextStyle() override {}
  void PopTextStyle() override {}
  void PushEventTarget(const text::EventTargetInfo&) override {}
  void PopEventTarget() override {}
  void SetTextStyle(TextPropertyKeyID, void*, size_t) override {}
  void AddText(const char*, size_t) override {}
  void AddInlineView(std::unique_ptr<text::InlineView>) override {}
  void AddImage(const char*, size_t) override {}
  void StartInlineTruncation() override {}
  void EndInlineTruncation() override {}
  void SetPlaceHolderStyle(TextPropertyKeyID, void*, size_t) override {}
  text::Paragraph* BuildParagraph() override {
    return state_.fail_paragraph
               ? nullptr
               : reinterpret_cast<text::Paragraph*>(&state_.paragraph_token);
  }

 private:
  BackendState& state_;
};

class FakeTextLayoutAPI : public text::TextLayoutAPI {
 public:
  explicit FakeTextLayoutAPI(BackendState& state) : state_(state) {}

  text::ParagraphBuilder* CreateParagraphBuilder() override {
    return state_.fail_builder ? nullptr : new FakeParagraphBuilder(state_);
  }
  void DestroyParagraphBuilder(text::ParagraphBuilder* builder) override {
    delete builder;
  }
  text::MeasureResult MeasureParagraph(text::Paragraph*,
                                       text::MeasureParams params) override {
    ++state_.measured_count;
    state_.params = params;
    return state_.result;
  }
  void AlignParagraph(text::Paragraph*, float, float) override {}
  text::Page* GetPage(text::Paragraph*) override {
    ++state_.created_page_count;
    ++state_.live_page_count;
    state_.last_page = reinterpret_cast<text::Page*>(new int(1));
    return state_.last_page;
  }
  void DestroyPage(text::Page* page) override {
    ++state_.destroyed_page_count;
    --state_.live_page_count;
    delete reinterpret_cast<int*>(page);
    if (state_.last_page == page) {
      state_.last_page = nullptr;
    }
  }
  void DestroyParagraph(text::Paragraph*) override {
    ++state_.destroyed_paragraph_count;
  }
  text::TextInfo GetTextInfo(std::string_view, float, std::string_view, float,
                             int) override {
    return {};
  }

 private:
  BackendState& state_;
};

class TextEventRangePaintingContext : public MockPaintingContext,
                                      public NativePaintingContext {
 public:
  void SetTextLayout(std::unique_ptr<TextLayoutImpl> layout) {
    text_layout_impl_ = std::move(layout);
  }
  TextLayoutImpl* text_layout() { return text_layout_impl_.get(); }
  NativePaintingContext* CastToNativeCtx() override { return this; }
  void FinishTasmOperation(const std::shared_ptr<PipelineOptions>&) override {}
  void FinishLayoutOperation(const std::shared_ptr<PipelineOptions>&) override {
  }
  void CreatePlatformRenderer(int, PlatformRendererType,
                              const fml::RefPtr<PropBundle>&,
                              const PlatformRendererInitConfig&) override {}
  void CreatePlatformExtendedRenderer(
      int, const base::String&, const fml::RefPtr<PropBundle>&,
      const PlatformRendererInitConfig&) override {}
  fml::RefPtr<PaintImage> CreateImage(int, base::String, const ImagePaintInfo&,
                                      float, float, int32_t, bool) override {
    return nullptr;
  }
  void UpdateTextBundle(int, intptr_t) override { ++bundle_updates; }
  void DestroyTextBundle(int) override { ++bundle_destroys; }
  void UpdatePlatformEventBundle(int, PlatformEventBundle) override {}
  void UpdateTextEventTargetRanges(
      int id, std::vector<PlatformTextEventTargetRange> ranges) override {
    last_text_id = id;
    range_updates.emplace_back(std::move(ranges));
  }
  void EnqueueDisplayList(int, DisplayList) override {}
  void EnqueueDisplayLists(DisplayListUpdateBatch) override {}
  void EnqueueReconstructEventTargetTreeRecursively() override {}

  int last_text_id{0};
  int bundle_updates{0};
  int bundle_destroys{0};
  std::vector<std::vector<PlatformTextEventTargetRange>> range_updates;
};

}  // namespace

class TextLayoutTextraTest : public ::testing::Test {
 public:
  void SetUp() override {
    api_ = new FakeTextLayoutAPI(state_);
    layout_ =
        std::make_unique<TextLayoutTextra>(reinterpret_cast<intptr_t>(api_));
  }

  FakeTextLayoutAPI* api() { return api_; }

  void ExpectUnavailable() {
    EXPECT_FALSE(
        layout_->MeasureDetached(&element_, 100, 2, 100, 2).has_value());
    const auto mounted_result = layout_->Measure(&element_, 100, 2, 100, 2);
    EXPECT_EQ(mounted_result.width_, 0);
    EXPECT_EQ(mounted_result.height_, 0);
    EXPECT_EQ(mounted_result.baseline_, 0);
  }

  fml::RefPtr<ComposeElementHandle> CreateDetachedComposeText() {
    auto context = std::make_unique<TextEventRangePaintingContext>();
    event_context_ = context.get();
    context->SetTextLayout(std::move(layout_));
    event_delegate_ =
        std::make_shared<::testing::NiceMock<test::MockTasmDelegate>>();
    PageOptions options;
    options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
    event_manager_ = std::make_unique<ElementManager>(
        std::move(context), event_delegate_.get(),
        LynxEnvConfig(1080, 1920, 1.f, 1.), options);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    event_manager_->SetConfig(config);
    event_manager_->CreateFiberPage("page", 11);
    return fml::AdoptRef(new ComposeElementHandle(event_manager_.get(),
                                                  ComposeElementKind::kText));
  }

  fml::RefPtr<Element> CreateTextWithEventRanges() {
    auto context = std::make_unique<TextEventRangePaintingContext>();
    event_context_ = context.get();
    event_delegate_ =
        std::make_shared<::testing::NiceMock<test::MockTasmDelegate>>();
    PageOptions options;
    options.SetEmbeddedMode(EmbeddedMode::FRAGMENT_LAYER_RENDER);
    event_manager_ = std::make_unique<ElementManager>(
        std::move(context), event_delegate_.get(),
        LynxEnvConfig(1080, 1920, 1.f, 1.), options);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    event_manager_->SetConfig(config);
    auto page = event_manager_->CreateFiberPage("page", 11);
    auto text = event_manager_->CreateFiberText("text");
    auto child = event_manager_->CreateFiberText("text");
    static_cast<TextElement*>(child.get())->content() = "abc";
    child->AddEventListener(
        "tap",
        std::make_shared<event::ClosureEventListener>([](lepus::Value) {}));
    text->InsertNode(child);
    // Populate the render children normally assembled by the flush pipeline.
    text->StoreLayoutNode(child.get(), nullptr);
    page->InsertNode(text);
    return text;
  }

  BackendState state_;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> event_delegate_;
  std::unique_ptr<ElementManager> event_manager_;
  TextEventRangePaintingContext* event_context_{nullptr};
  DetachedTextElement element_;
  FakeTextLayoutAPI* api_{nullptr};
  std::unique_ptr<TextLayoutTextra> layout_;
};

TEST(TextLayoutTextraUnavailableTest, MissingBackendReturnsFailure) {
  DetachedTextElement element;
  TextLayoutTextra layout(0);
  layout.DispatchLayoutBefore(&element);
  EXPECT_FALSE(layout.MeasureDetached(&element, 100, 2, 100, 2).has_value());
  const auto mounted_result = layout.Measure(&element, 100, 2, 100, 2);
  EXPECT_EQ(mounted_result.width_, 0);
  EXPECT_EQ(mounted_result.height_, 0);
  EXPECT_EQ(mounted_result.baseline_, 0);
}

TEST_F(TextLayoutTextraTest, MissingParagraphReturnsFailure) {
  ExpectUnavailable();
  EXPECT_EQ(state_.measured_count, 0);
}

TEST_F(TextLayoutTextraTest, ZeroSizeIsSuccessful) {
  layout_->DispatchLayoutBefore(&element_);
  const auto result = layout_->MeasureDetached(&element_, 100, 2, 100, 2);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->width_, 0);
  EXPECT_EQ(result->height_, 0);
  EXPECT_EQ(result->baseline_, 0);
  EXPECT_EQ(state_.measured_count, 1);
  EXPECT_EQ(state_.live_builder_count, 0);
}

TEST_F(TextLayoutTextraTest, PreservesBackendMetricsAndConstraints) {
  state_.result = {37.5f, 21.25f, 16.75f};
  layout_->DispatchLayoutBefore(&element_);
  const auto result = layout_->MeasureDetached(&element_, 100, 2, 200, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->width_, 37.5f);
  EXPECT_EQ(result->height_, 21.25f);
  EXPECT_EQ(result->baseline_, 16.75f);
  EXPECT_EQ(state_.params.width, 100);
  EXPECT_EQ(state_.params.width_mode, text::LayoutMode::kAtMost);
  EXPECT_EQ(state_.params.height, 200);
  EXPECT_EQ(state_.params.height_mode, text::LayoutMode::kDefinite);
}

TEST_F(TextLayoutTextraTest, DetachedMeasurementDoesNotCreateTextPages) {
  // Mirror ComposeElementHandle::MeasureText: prepare, measure, then destroy.
  // Repeated detached measurements must not leak a Page per round: the detached
  // element never reaches OnUpdateLayout(), so nobody would take over a
  // published page.
  for (int i = 0; i < 100; ++i) {
    layout_->DispatchLayoutBefore(&element_);
    ASSERT_TRUE(
        layout_->MeasureDetached(&element_, 100, 2, 100, 2).has_value());
    layout_->DestroyDetached(&element_);
  }
  EXPECT_EQ(state_.created_page_count, 0);
  EXPECT_EQ(state_.destroyed_page_count, 0);
  EXPECT_EQ(state_.live_page_count, 0);
  EXPECT_EQ(state_.destroyed_paragraph_count, 100);
}

TEST_F(TextLayoutTextraTest, DetachedMeasurementDoesNotDestroyPlatformBundles) {
  auto handle = CreateDetachedComposeText();
  ASSERT_FALSE(handle->content_element()->EnableFragmentLayerRender());
  const char* error = nullptr;
  for (int i = 0; i < 3; ++i) {
    ASSERT_TRUE(
        handle->MeasureText(event_manager_.get(), 100, 2, 100, 2, &error)
            .has_value());
  }
  EXPECT_EQ(state_.created_page_count, 0);
  EXPECT_EQ(state_.destroyed_paragraph_count, 3);
  EXPECT_EQ(state_.live_builder_count, 0);
  EXPECT_EQ(event_context_->bundle_updates, 0);
  EXPECT_EQ(event_context_->bundle_destroys, 0);
  EXPECT_TRUE(event_context_->range_updates.empty());

  state_.fail_paragraph = true;
  EXPECT_FALSE(handle->MeasureText(event_manager_.get(), 100, 2, 100, 2, &error)
                   .has_value());
  EXPECT_EQ(state_.live_builder_count, 0);
  EXPECT_EQ(event_context_->bundle_destroys, 0);
}

TEST_F(TextLayoutTextraTest, DetachedCleanupPreservesPublishedPlatformBundle) {
  auto handle = CreateDetachedComposeText();
  auto* element = handle->content_element().get();
  auto* layout = event_context_->text_layout();
  // Ordinary measurement hands a Page to the non-fragment platform pipeline.
  static_cast<TextElement*>(element)->PrepareForDetachedMeasurement();
  layout->DispatchLayoutBefore(element);
  layout->Measure(element, 100, 2, 100, 2);
  ASSERT_EQ(state_.created_page_count, 1);
  EXPECT_EQ(event_context_->bundle_updates, 1);

  const char* error = nullptr;
  ASSERT_TRUE(handle->MeasureText(event_manager_.get(), 100, 2, 100, 2, &error)
                  .has_value());
  EXPECT_EQ(state_.created_page_count, 1);
  EXPECT_EQ(event_context_->bundle_destroys, 0);
  EXPECT_EQ(state_.destroyed_paragraph_count, 2);

  // Full teardown must still release the published platform bundle.
  layout->Destroy(element);
  EXPECT_EQ(event_context_->bundle_destroys, 1);
  api()->DestroyPage(state_.last_page);
  EXPECT_EQ(state_.live_page_count, 0);
}

TEST_F(TextLayoutTextraTest, MountedMeasureStillPublishesTextPage) {
  layout_->DispatchLayoutBefore(&element_);
  const auto result = layout_->Measure(&element_, 100, 2, 100, 2);
  EXPECT_EQ(result.width_, 0);
  EXPECT_EQ(state_.created_page_count, 1);
  ASSERT_NE(state_.last_page, nullptr);
  EXPECT_EQ(state_.live_page_count, 1);
  // Mounted measurement hands the page to the platform text pipeline; emulate
  // the platform destroying its bundle.
  api()->DestroyPage(state_.last_page);
  EXPECT_EQ(state_.live_page_count, 0);
}

TEST_F(TextLayoutTextraTest, BuilderFailureDiscardsPreviousParagraph) {
  layout_->DispatchLayoutBefore(&element_);
  ASSERT_TRUE(layout_->MeasureDetached(&element_, 100, 2, 100, 2).has_value());

  state_.fail_builder = true;
  layout_->DispatchLayoutBefore(&element_);
  ExpectUnavailable();
  EXPECT_EQ(state_.measured_count, 1);
  EXPECT_EQ(state_.destroyed_paragraph_count, 1);
  EXPECT_EQ(state_.live_builder_count, 0);

  state_.fail_builder = false;
  layout_->DispatchLayoutBefore(&element_);
  EXPECT_TRUE(layout_->MeasureDetached(&element_, 100, 2, 100, 2).has_value());
}

TEST_F(TextLayoutTextraTest, ParagraphFailureDiscardsPreviousParagraph) {
  layout_->DispatchLayoutBefore(&element_);
  ASSERT_TRUE(layout_->MeasureDetached(&element_, 100, 2, 100, 2).has_value());

  state_.fail_paragraph = true;
  layout_->DispatchLayoutBefore(&element_);
  ExpectUnavailable();
  EXPECT_EQ(state_.measured_count, 1);
  EXPECT_EQ(state_.destroyed_paragraph_count, 1);
  EXPECT_EQ(state_.live_builder_count, 0);

  state_.fail_paragraph = false;
  layout_->DispatchLayoutBefore(&element_);
  EXPECT_TRUE(layout_->MeasureDetached(&element_, 100, 2, 100, 2).has_value());
}

TEST_F(TextLayoutTextraTest, PreparationFailureClearsPublishedEventRanges) {
  auto text = CreateTextWithEventRanges();
  for (auto failure :
       {&BackendState::fail_builder, &BackendState::fail_paragraph}) {
    layout_->DispatchLayoutBefore(text.get());
    ASSERT_FALSE(event_context_->range_updates.empty());
    ASSERT_FALSE(event_context_->range_updates.back().empty());
    const auto published_count = event_context_->range_updates.size();

    state_.*failure = true;
    layout_->DispatchLayoutBefore(text.get());
    ASSERT_EQ(event_context_->range_updates.size(), published_count + 1);
    EXPECT_TRUE(event_context_->range_updates.back().empty());
    EXPECT_EQ(event_context_->last_text_id, text->impl_id());
    layout_->Destroy(text.get());
    EXPECT_EQ(event_context_->range_updates.size(), published_count + 1);
    state_.*failure = false;
  }
}

TEST_F(TextLayoutTextraTest, DestroyClearsPublishedEventRangesOnce) {
  auto text = CreateTextWithEventRanges();
  layout_->DispatchLayoutBefore(text.get());
  ASSERT_EQ(event_context_->range_updates.size(), 1u);
  ASSERT_FALSE(event_context_->range_updates.back().empty());
  layout_->Destroy(text.get());
  ASSERT_EQ(event_context_->range_updates.size(), 2u);
  EXPECT_TRUE(event_context_->range_updates.back().empty());
  EXPECT_EQ(event_context_->last_text_id, text->impl_id());
  layout_->Destroy(text.get());
  EXPECT_EQ(event_context_->range_updates.size(), 2u);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

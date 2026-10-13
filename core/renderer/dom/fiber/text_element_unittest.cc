// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/fiber/text_element.h"

#include <array>
#include <limits>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/element_utils.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_text_layout.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/lynx_ui_operation_queue.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_layout_platform.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class TextElementTest : public FiberElementTest {};

namespace {

// Keep the shared mock's zero-size default unchanged. This probe exists only
// for detached measurement tests, where native calls must be observable.
class DetachedTextLayoutProbe : public TextLayoutMock {
 public:
  void DispatchLayoutBefore(Element* element) override {
    calls.emplace_back("dispatch");
    auto* text = static_cast<TextElement*>(element);
    dispatched_text = text->content().str();
    dispatched_font_size = text->computed_css_style()->GetFontSize();
    dispatched_max_lines =
        text->text_props() ? text->text_props()->text_max_line : std::nullopt;
    if (text->computed_css_style()->GetTextAttributes().has_value()) {
      dispatched_letter_spacing =
          text->computed_css_style()->GetTextAttributes()->letter_spacing;
    }
    TextLayoutMock::DispatchLayoutBefore(element);
  }

  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode) override {
    calls.emplace_back("measure");
    constraints = {width, static_cast<float>(width_mode), height,
                   static_cast<float>(height_mode)};
    EXPECT_TRUE(text_layout_results_.count(element->impl_id()));
    return result;
  }

  std::optional<LayoutResult> MeasureDetached(Element* element, float width,
                                              int width_mode, float height,
                                              int height_mode) override {
    const auto measured =
        Measure(element, width, width_mode, height, height_mode);
    return native_available ? std::optional<LayoutResult>(measured)
                            : std::nullopt;
  }

  void Align(Element*) override { calls.emplace_back("align"); }

  void Destroy(Element* element) override {
    calls.emplace_back("destroy");
    text_layout_results_.erase(element->impl_id());
  }

  std::vector<std::string> calls;
  std::array<float, 4> constraints{};
  LayoutResult result{37.5f, 21.25f, 16.75f};
  std::string dispatched_text;
  double dispatched_font_size{0};
  double dispatched_letter_spacing{0};
  std::optional<int32_t> dispatched_max_lines;
  bool native_available{true};
};

}  // namespace

class DetachedTextMeasurementTest : public FiberElementTest {
 public:
  void SetUp() override {
    FiberElementTest::SetUp();
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);
    manager->page_options_.embedded_mode_ = EmbeddedMode::LAYOUT_IN_ELEMENT;
    manager->SetCurrentEngineThreadId(std::this_thread::get_id());
    auto probe = std::make_unique<DetachedTextLayoutProbe>();
    probe_ = probe.get();
    platform_impl_->text_layout_impl_ = std::move(probe);
    platform_impl_->ResetFlushFlag();
    manager->need_layout_ = false;
  }

  fml::RefPtr<ComposeElementHandle> CreateText() {
    return fml::AdoptRef(
        new ComposeElementHandle(manager, ComposeElementKind::kText));
  }

  DetachedTextLayoutProbe* probe_{nullptr};
};

TEST_P(DetachedTextMeasurementTest, PreparesMeasuresDestroysWithoutFlush) {
  for (bool new_styling : {false, true}) {
    manager->enable_new_styling_pipeline_ = new_styling;
    auto handle = CreateText();
    auto* text = static_cast<TextElement*>(handle->content_element().get());
    text->SetAttribute("text", lepus::Value("first"));
    text->SetAttribute("text-maxline", lepus::Value(1));
    text->SetRawInlineStyles(base::String("font-size:20px;letter-spacing:2px"));
    // A layout request, even on the detached node, is forbidden.
    text->EnsureSLNode();
    int requests = 0;
    auto* original_context = text->slnode()->GetContext();
    text->slnode()->SetContext(&requests);
    text->slnode()->SetSLRequestLayoutFunc(
        [](void* context) { ++*static_cast<int*>(context); });
    const char* error = nullptr;
    probe_->calls.clear();
    auto first = handle->MeasureText(manager, 120.5, 2, 80.25, 1, &error);
    ASSERT_TRUE(first.has_value()) << (error ? error : "");
    EXPECT_EQ(first->width_, 37.5f);
    EXPECT_EQ(first->height_, 21.25f);
    EXPECT_EQ(first->baseline_, 16.75f);
    EXPECT_EQ(probe_->dispatched_text, "first");
    EXPECT_EQ(probe_->dispatched_max_lines, 1);
    EXPECT_DOUBLE_EQ(probe_->dispatched_font_size, 20);
    EXPECT_EQ(probe_->constraints,
              (std::array<float, 4>{120.5f, 2, 80.25f, 1}));
    EXPECT_EQ(probe_->calls,
              (std::vector<std::string>{"dispatch", "measure", "destroy"}));
    EXPECT_TRUE(probe_->text_layout_results_.empty());
    EXPECT_TRUE(text->updated_attr_map_.empty());

    text->SetAttribute("text", lepus::Value("updated"));
    text->SetAttribute("text-maxline", lepus::Value(2));
    text->SetRawInlineStyles(base::String("font-size:28px"));
    probe_->result = LayoutResult{51, 32, 23};
    probe_->calls.clear();
    auto second = handle->MeasureText(manager, 0, 0, 0, 0, &error);
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->width_, 51);
    EXPECT_EQ(probe_->dispatched_text, "updated");
    EXPECT_EQ(probe_->dispatched_max_lines, 2);
    EXPECT_DOUBLE_EQ(probe_->dispatched_font_size, 28);
    EXPECT_EQ(first->width_, 37.5f);  // Independent numeric snapshot.
    EXPECT_EQ(probe_->calls,
              (std::vector<std::string>{"dispatch", "measure", "destroy"}));
    text->RemoveAllInlineStyles();
    ASSERT_TRUE(handle->MeasureText(manager, 100, 2, 100, 2, &error));
    EXPECT_DOUBLE_EQ(probe_->dispatched_font_size,
                     manager->GetLynxEnvConfig().PageDefaultFontSize());
    EXPECT_EQ(text->slnode()->GetCSSStyle(),
              text->computed_css_style()->GetLayoutComputedStyle());
    EXPECT_EQ(requests, 0);
    EXPECT_FALSE(manager->need_layout_);
    EXPECT_FALSE(platform_impl_->HasFlushed());
    EXPECT_TRUE(platform_impl_->captured_create_tags_map_.empty());
    EXPECT_TRUE(platform_impl_->node_map_.empty());
    EXPECT_TRUE(platform_impl_->queue_->operations_.Empty());
    EXPECT_TRUE(platform_impl_->queue_->high_priority_operations_.Empty());
    EXPECT_TRUE(tasm_mediator.captured_bundles_.empty());
    probe_->result = LayoutResult{37.5f, 21.25f, 16.75f};
    text->slnode()->SetSLRequestLayoutFunc(nullptr);
    text->slnode()->SetContext(original_context);
  }
}

TEST_P(DetachedTextMeasurementTest, PassesEverySLModeAndRejectsInvalidNumbers) {
  auto handle = CreateText();
  const char* error = nullptr;
  for (int width_mode = 0; width_mode <= 2; ++width_mode) {
    for (int height_mode = 0; height_mode <= 2; ++height_mode) {
      auto result = handle->MeasureText(manager, 10.5, width_mode, 20.5,
                                        height_mode, &error);
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(probe_->constraints,
                (std::array<float, 4>{10.5f, static_cast<float>(width_mode),
                                      20.5f, static_cast<float>(height_mode)}));
    }
  }
  probe_->calls.clear();
  const double invalid_sizes[] = {-1, std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::quiet_NaN(),
                                  std::numeric_limits<double>::max()};
  for (double size : invalid_sizes) {
    EXPECT_FALSE(handle->MeasureText(manager, size, 0, 10, 0, &error));
    EXPECT_FALSE(handle->MeasureText(manager, 10, 0, size, 0, &error));
  }
  const double invalid_modes[] = {-1,
                                  3,
                                  0.5,
                                  1.5,
                                  std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::quiet_NaN()};
  for (double mode : invalid_modes) {
    EXPECT_FALSE(handle->MeasureText(manager, 10, mode, 10, 0, &error));
    EXPECT_FALSE(handle->MeasureText(manager, 10, 0, 10, mode, &error));
  }
  EXPECT_TRUE(probe_->calls.empty());
}

TEST_P(DetachedTextMeasurementTest, RejectsWrongOwnershipKindAndTree) {
  const char* error = nullptr;
  auto handle = CreateText();
  auto measure = [&](ComposeElementHandle* candidate) {
    return candidate->MeasureText(manager, 100, 2, 100, 2, &error);
  };
  EXPECT_FALSE(handle->MeasureText(nullptr, 100, 2, 100, 2, &error));
  auto view = fml::AdoptRef(
      new ComposeElementHandle(manager, ComposeElementKind::kView));
  auto image = fml::AdoptRef(
      new ComposeElementHandle(manager, ComposeElementKind::kImage));
  EXPECT_FALSE(measure(view.get()));
  EXPECT_FALSE(measure(image.get()));
  manager->SetCurrentEngineThreadId(std::thread::id{});
  EXPECT_FALSE(measure(handle.get()));
  manager->engine_thread_id_.reset();
  EXPECT_FALSE(measure(handle.get()));
  manager->SetCurrentEngineThreadId(std::this_thread::get_id());
  manager->page_options_.embedded_mode_ = EmbeddedMode::UNSET;
  EXPECT_FALSE(measure(handle.get()));
  manager->page_options_.embedded_mode_ = EmbeddedMode::LAYOUT_IN_ELEMENT;

  std::optional<LayoutResult> foreign_thread_result;
  std::thread foreign_thread(
      [&]() { foreign_thread_result = measure(handle.get()); });
  foreign_thread.join();
  EXPECT_FALSE(foreign_thread_result.has_value());
  auto other_manager = std::make_unique<ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), &tasm_mediator,
      manager->GetLynxEnvConfig(), PageOptions(), report::kUnknownInstanceId,
      vsync_monitor_, std::make_unique<test::MockPlatformImpl>());
  EXPECT_FALSE(
      handle->MeasureText(other_manager.get(), 100, 2, 100, 2, &error));
  handle->content_element()->MarkAttached();
  EXPECT_FALSE(measure(handle.get()));
  handle->content_element()->MarkDetached();

  auto page = manager->CreateFiberPage("page", 11);
  page->InsertNode(handle->content_element());
  EXPECT_FALSE(measure(handle.get()));
  auto childless = CreateText();
  auto child = manager->CreateFiberRawText();
  childless->content_element()->InsertNode(child);
  EXPECT_FALSE(measure(childless.get()));
  auto inline_text = CreateText();
  inline_text->content_element()->ConvertToInlineElement();
  EXPECT_FALSE(measure(inline_text.get()));
  auto wrapped = CreateText();
  wrapped->SetMountRoot(manager->CreateFiberView());
  EXPECT_FALSE(measure(wrapped.get()));
  EXPECT_TRUE(probe_->calls.empty());
}

TEST_P(DetachedTextMeasurementTest, InvalidNativeResultStillDestroysCache) {
  auto handle = CreateText();
  const char* error = nullptr;
  probe_->result.width_ = std::numeric_limits<float>::quiet_NaN();
  EXPECT_FALSE(handle->MeasureText(manager, 100, 2, 100, 2, &error));
  EXPECT_EQ(probe_->calls,
            (std::vector<std::string>{"dispatch", "measure", "destroy"}));
  EXPECT_TRUE(probe_->text_layout_results_.empty());
}

TEST_P(DetachedTextMeasurementTest, ResolvesFontFacesLocallyBeforeDispatch) {
  CSSFontFaceRuleMap font_faces;
  font_faces["custom-font"].push_back(std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom-font.woff2)"}}));
  auto fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, CSSKeyframesTokenMap{},
      std::move(font_faces));
  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(fragment)));
  manager->AdoptStyleSheet(wrapper);
  auto handle = CreateText();
  auto* text = static_cast<TextElement*>(handle->content_element().get());
  text->SetAttribute("text", lepus::Value("font"));
  text->SetRawInlineStyles(
      base::String("font-family:custom-font;font-size:20px"));
  ASSERT_FALSE(wrapper->HasFontFacesResolved());
  const char* error = nullptr;
  ASSERT_TRUE(handle->MeasureText(manager, 100, 2, 100, 2, &error));
  EXPECT_TRUE(wrapper->HasFontFacesResolved());
  EXPECT_TRUE(text->property_bits().Has(kPropertyIDFontFamily));
  ASSERT_TRUE(text->computed_css_style()->GetTextAttributes().has_value());
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->font_family,
            base::String("custom-font"));
  EXPECT_FALSE(manager->need_layout_);
  EXPECT_FALSE(platform_impl_->HasFlushed());
  EXPECT_TRUE(platform_impl_->queue_->operations_.Empty());
  EXPECT_TRUE(platform_impl_->queue_->high_priority_operations_.Empty());
}

TEST_P(DetachedTextMeasurementTest, BindingRejectsWrongHandleBeforeCast) {
  const lepus::Value wrong_handles[] = {
      lepus::Value(), lepus::Value(1), lepus::Value("not a handle"),
      lepus::Value(lepus::Dictionary::Create()),
      lepus::Value(manager->CreateFiberText("text"))};
  for (const auto& wrong : wrong_handles) {
    lepus::Value args[] = {wrong, lepus::Value(100), lepus::Value(2),
                           lepus::Value(100), lepus::Value(2)};
    base::ErrorStorage::GetInstance().Reset();
    EXPECT_TRUE(
        RendererFunctions::FiberMeasureTextElement(nullptr, args, 5).IsEmpty());
    EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  }
  auto handle = CreateText();
  lepus::Value args[] = {lepus::Value(handle), lepus::Value("100"),
                         lepus::Value(2), lepus::Value(100), lepus::Value(2)};
  base::ErrorStorage::GetInstance().Reset();
  EXPECT_TRUE(
      RendererFunctions::FiberMeasureTextElement(nullptr, args, 5).IsEmpty());
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  EXPECT_TRUE(
      RendererFunctions::FiberMeasureTextElement(nullptr, args, 4).IsEmpty());
  EXPECT_TRUE(probe_->calls.empty());
}

TEST_P(DetachedTextMeasurementTest, BindingReturnsNumericSnapshotOrUndefined) {
  auto renderer_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_NE(renderer_runtime, nullptr);
  renderer_runtime->Initialize();
  renderer_runtime->SetGlobalData(
      BASE_STATIC_STRING(kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* context = runtime::MTSRuntime::ToQuickContext(renderer_runtime.get());
  ASSERT_NE(context, nullptr);
  auto handle = CreateText();
  lepus::Value args[] = {lepus::Value(handle), lepus::Value(100),
                         lepus::Value(2), lepus::Value(100), lepus::Value(2)};
  base::ErrorStorage::GetInstance().Reset();
  auto first = RendererFunctions::FiberMeasureTextElement(context, args, 5);
  ASSERT_TRUE(first.IsObject());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  EXPECT_TRUE(first.GetProperty("width").IsNumber());
  EXPECT_TRUE(first.GetProperty("height").IsNumber());
  EXPECT_TRUE(first.GetProperty("baseline").IsNumber());
  EXPECT_DOUBLE_EQ(first.GetProperty("width").Number(), 37.5);
  EXPECT_DOUBLE_EQ(first.GetProperty("height").Number(), 21.25);
  EXPECT_DOUBLE_EQ(first.GetProperty("baseline").Number(), 16.75);
  probe_->result = LayoutResult{50, 30, 20};
  auto second = RendererFunctions::FiberMeasureTextElement(context, args, 5);
  EXPECT_DOUBLE_EQ(second.GetProperty("width").Number(), 50);
  EXPECT_DOUBLE_EQ(first.GetProperty("width").Number(), 37.5);
  probe_->calls.clear();
  args[2] = lepus::Value(1.5);
  EXPECT_TRUE(
      RendererFunctions::FiberMeasureTextElement(context, args, 5).IsEmpty());
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  EXPECT_TRUE(probe_->calls.empty());
}

TEST_P(DetachedTextMeasurementTest,
       NativeFailureRecoveryAndLegalZeroThroughBindingUnderFLR) {
  auto renderer_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_NE(renderer_runtime, nullptr);
  renderer_runtime->Initialize();
  renderer_runtime->SetGlobalData(
      BASE_STATIC_STRING(kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* context = runtime::MTSRuntime::ToQuickContext(renderer_runtime.get());
  ASSERT_NE(context, nullptr);
  for (bool fragment_layer : {false, true}) {
    manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
        fragment_layer ? EmbeddedMode::LAYOUT_IN_ELEMENT |
                             EmbeddedMode::FRAGMENT_LAYER_RENDER
                       : EmbeddedMode::LAYOUT_IN_ELEMENT);
    auto handle = CreateText();
    handle->content_element()->SetAttribute("text", lepus::Value("IR text"));
    auto font = lepus::Dictionary::Create();
    font->SetValue("op", lepus::Value(3));
    font->SetValue("propertyId",
                   lepus::Value(static_cast<int32_t>(kPropertyIDFontSize)));
    font->SetValue("value", lepus::Value("28px"));
    auto spacing = lepus::Dictionary::Create();
    spacing->SetValue("op", lepus::Value(3));
    spacing->SetValue(
        "propertyId",
        lepus::Value(static_cast<int32_t>(kPropertyIDLetterSpacing)));
    spacing->SetValue("value", lepus::Value("3px"));
    spacing->SetValue("previous", lepus::Value(font));
    lepus::Value modifier_args[] = {lepus::Value(handle),
                                    lepus::Value(spacing)};
    base::ErrorStorage::GetInstance().Reset();
    RendererFunctions::FiberSetComposeModifier(context, modifier_args, 2);
    ASSERT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
    ASSERT_EQ(handle->mount_root(), handle->content_element());

    lepus::Value args[] = {lepus::Value(handle), lepus::Value(100),
                           lepus::Value(2), lepus::Value(100), lepus::Value(2)};
    probe_->calls.clear();
    probe_->native_available = false;
    auto failed = RendererFunctions::FiberMeasureTextElement(context, args, 5);
    EXPECT_TRUE(failed.IsEmpty());
    EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
    EXPECT_EQ(probe_->calls,
              (std::vector<std::string>{"dispatch", "measure", "destroy"}));
    EXPECT_TRUE(probe_->text_layout_results_.empty());

    probe_->calls.clear();
    probe_->native_available = true;
    probe_->result = LayoutResult{50, 30, 20};
    base::ErrorStorage::GetInstance().Reset();
    auto recovered =
        RendererFunctions::FiberMeasureTextElement(context, args, 5);
    ASSERT_TRUE(recovered.IsObject());
    EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
    EXPECT_DOUBLE_EQ(recovered.GetProperty("width").Number(), 50);
    EXPECT_EQ(probe_->dispatched_text, "IR text");
    EXPECT_DOUBLE_EQ(probe_->dispatched_font_size, 28);
    EXPECT_DOUBLE_EQ(probe_->dispatched_letter_spacing, 3);
    EXPECT_EQ(probe_->calls,
              (std::vector<std::string>{"dispatch", "measure", "destroy"}));
    EXPECT_TRUE(probe_->text_layout_results_.empty());

    probe_->calls.clear();
    probe_->result = LayoutResult{0, 0, 0};
    auto zero = RendererFunctions::FiberMeasureTextElement(context, args, 5);
    ASSERT_TRUE(zero.IsObject());
    EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
    EXPECT_DOUBLE_EQ(zero.GetProperty("width").Number(), 0);
    EXPECT_DOUBLE_EQ(zero.GetProperty("height").Number(), 0);
    EXPECT_DOUBLE_EQ(zero.GetProperty("baseline").Number(), 0);
    EXPECT_DOUBLE_EQ(recovered.GetProperty("width").Number(), 50);
    EXPECT_EQ(probe_->calls,
              (std::vector<std::string>{"dispatch", "measure", "destroy"}));
    EXPECT_TRUE(probe_->text_layout_results_.empty());
    EXPECT_FALSE(manager->need_layout_);
    EXPECT_FALSE(platform_impl_->HasFlushed());
    EXPECT_TRUE(platform_impl_->captured_create_tags_map_.empty());
    EXPECT_TRUE(platform_impl_->node_map_.empty());
    EXPECT_TRUE(platform_impl_->queue_->operations_.Empty());
    EXPECT_TRUE(platform_impl_->queue_->high_priority_operations_.Empty());
  }
}

INSTANTIATE_TEST_SUITE_P(DetachedTextMeasurementTestModule,
                         DetachedTextMeasurementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

TEST(TextPropsTest, TextDecorationExtensionPropertiesAreWanted) {
  EXPECT_EQ(IsTextMeasurerWanted(kPropertyIDTextDecorationThickness), 1);
  EXPECT_EQ(IsTextMeasurerWanted(kPropertyIDXTextDecorationWidth), 1);
  EXPECT_EQ(IsTextMeasurerWanted(kPropertyIDXTextDecorationGap), 1);
}

TEST(TextPropsTest, DefaultDetachedMeasurementPreservesNormalZeroResult) {
  TextLayoutMock mock;
  auto measured = mock.MeasureDetached(nullptr, 100, 2, 100, 2);
  ASSERT_TRUE(measured.has_value());
  EXPECT_EQ(measured->width_, 0);
  EXPECT_EQ(measured->height_, 0);
  EXPECT_EQ(measured->baseline_, 0);
}

TEST_P(TextElementTest, TestInlineText) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  // inline text
  auto inline_text = manager->CreateFiberText("text");

  auto inline_raw_text = manager->CreateFiberRawText();
  auto inline_content = lepus::Value("inline-text-content");
  inline_raw_text->SetText(inline_content);
  inline_text->InsertNode(inline_raw_text);
  text->InsertNode(inline_text);

  page->FlushActionsAsRoot();

  auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  // check element tree
  EXPECT_TRUE(text->GetTag() == "text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->GetTag() == "inline-text");
}

TEST_P(TextElementTest, TestXInlineText) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("x-text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  // inline text
  auto inline_text = manager->CreateFiberText("x-text");

  auto inline_raw_text = manager->CreateFiberRawText();
  auto inline_content = lepus::Value("inline-text-content");
  inline_raw_text->SetText(inline_content);
  inline_text->InsertNode(inline_raw_text);
  text->InsertNode(inline_text);

  page->FlushActionsAsRoot();

  auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  // check element tree
  EXPECT_EQ(text->GetTag(), "x-text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_EQ(inline_text->GetTag(), "x-inline-text");
}

TEST_P(TextElementTest, TestInlineTextAndImage) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  // inline text
  auto inline_text = manager->CreateFiberText("text");

  auto inline_raw_text = manager->CreateFiberRawText();
  auto inline_content = lepus::Value("inline-text-content");
  inline_raw_text->SetText(inline_content);
  inline_text->InsertNode(inline_raw_text);
  text->InsertNode(inline_text);

  // inline image
  auto inline_image = manager->CreateFiberImage("image");

  auto image_src = lepus::Value("inline-image-src://");
  inline_image->SetAttribute("src", image_src);
  text->InsertNode(inline_image);

  page->FlushActionsAsRoot();

  auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  auto& inline_image_attributes = inline_image->data_model_->attributes();
  EXPECT_TRUE(inline_image_attributes.at("src") ==
              lepus::Value("inline-image-src://"));

  // check element tree
  EXPECT_TRUE(text->GetTag() == "text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->GetTag() == "inline-text");
  EXPECT_TRUE(inline_text->parent() == text.get());

  EXPECT_TRUE(inline_image->is_inline_element());
  EXPECT_TRUE(inline_image->GetTag() == "inline-image");
  EXPECT_TRUE(inline_image->parent() == text.get());
}

TEST_P(TextElementTest, TestSetTextOverflow) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  text->SetAttribute("text-overflow", lepus::Value("ellipsis"));

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_painting_node_ =
      static_cast<FiberMockPaintingContext*>(
          manager->painting_context()->platform_impl_.get())
          ->node_map_.at(text->impl_id())
          .get();

  EXPECT_TRUE(mock_text_painting_node_->props_.size() == 1);
  std::string key("text-overflow");
  EXPECT_TRUE(
      mock_text_painting_node_->props_.at(key) ==
      lepus::Value(static_cast<int>(starlight::TextOverflowType::kEllipsis)));

  text->SetAttribute("text-overflow", lepus::Value("clip"));
  text->SetAttribute("layout-only", lepus::Value("false"));

  text->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(mock_text_painting_node_->props_.size() == 2);
  EXPECT_TRUE(
      mock_text_painting_node_->props_.at(key) ==
      lepus::Value(static_cast<int>(starlight::TextOverflowType::kClip)));
}

TEST_P(TextElementTest, AttributeStyleCacheMirrorsCommittedStyleCache) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto text = manager->CreateFiberText("text");
  text->SetAttributeInternal("text-overflow", lepus::Value("ellipsis"));

  const auto* cached_styles = text->PeekCachedStylesFromAttributes();
  ASSERT_NE(cached_styles, nullptr);
  auto cached_it = cached_styles->find(CSSPropertyID::kPropertyIDTextOverflow);
  ASSERT_TRUE(cached_it != cached_styles->end());
  EXPECT_EQ(cached_it->second,
            CSSValue(starlight::TextOverflowType::kEllipsis));

  const auto* committed_styles = text->PeekCommittedStylesFromAttributes();
  ASSERT_NE(committed_styles, nullptr);
  auto committed_it =
      committed_styles->find(CSSPropertyID::kPropertyIDTextOverflow);
  ASSERT_TRUE(committed_it != committed_styles->end());
  EXPECT_EQ(committed_it->second,
            CSSValue(starlight::TextOverflowType::kEllipsis));

  text->ResetAttribute("text-overflow");
  EXPECT_EQ(text->PeekCachedStylesFromAttributes(), nullptr);
  EXPECT_EQ(text->PeekCommittedStylesFromAttributes(), nullptr);
}

TEST_P(TextElementTest, TestConvertContent) {
  EXPECT_EQ(ConvertTextContent(lepus::Value("test")), base::String("test"));
  EXPECT_EQ(ConvertTextContent(lepus::Value((int32_t)1)), base::String("1"));
  EXPECT_EQ(ConvertTextContent(lepus::Value(uint32_t{4294967295U})),
            base::String("4294967295"));
  EXPECT_EQ(ConvertTextContent(lepus::Value((int64_t)11231212121212)),
            base::String("11231212121212"));
  EXPECT_EQ(ConvertTextContent(lepus::Value(uint64_t{18446744073709551615ULL})),
            base::String("18446744073709551615"));
  EXPECT_EQ(ConvertTextContent(lepus::Value(1.00)), base::String("1"));
  EXPECT_EQ(ConvertTextContent(lepus::Value(1.10)), base::String("1.1"));
  EXPECT_EQ(ConvertTextContent(lepus::Value(1.1)), base::String("1.1"));
  EXPECT_EQ(ConvertTextContent(lepus::Value()), base::String("null"));
}

TEST_P(TextElementTest, TestResolveStyleValue) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  text->SetAttribute("text-maxline", lepus::Value(1));
  text->SetRawInlineStyles(base::String("color:red;"));

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* mock_text_painting_node_ =
      painting_context->node_map_.at(text->impl_id()).get();

  EXPECT_EQ(mock_text_painting_node_->props_.size(), 1);
  std::string key("text-overflow");
  EXPECT_EQ(text->text_props_->text_max_line, 1);
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDColor));
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->color, 4294901760);

  text->SetAttribute("text-maxline", lepus::Value(2));
  text->SetAttribute("layout-only", lepus::Value("false"));
  text->RemoveAllInlineStyles();
  text->SetRawInlineStyles(base::String(""));

  text->FlushActionsAsRoot();

  EXPECT_EQ(text->text_props_->text_max_line, 2);
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDColor));
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->color, 4278190080);
}

TEST_P(TextElementTest, WordBreakReachesTextMeasurerUnderLayoutInElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  // word-break must be recognized as a text-measurer-wanted property. Otherwise
  // it is never recorded into property_bits_ under LayoutInElement, and the
  // platform text layout (e.g. TextLayoutDarwin::ApplyTextStyle) iterates
  // property_bits_ and therefore never consumes word-break. That is exactly why
  // word-break:break-all had no effect on iOS text when LayoutInElement was on.
  EXPECT_EQ(IsTextMeasurerWanted(kPropertyIDWordBreak), 1);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  raw_text->SetText(lepus::Value("text-content"));

  page->InsertNode(text);
  text->InsertNode(raw_text);

  text->SetRawInlineStyles(base::String("word-break:break-all;"));

  page->FlushActionsAsRoot();

  // Under LayoutInElement, word-break must be flagged in property_bits_ so the
  // platform text layout iterates over it, and the resolved value must be
  // break-all.
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDWordBreak));
  ASSERT_TRUE(text->computed_css_style()->GetTextAttributes().has_value());
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->word_break,
            starlight::WordBreakType::kBreakAll);
}

TEST_P(TextElementTest, ResetTextMaxlineClearsLayoutInElementProp) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  raw_text->SetText(lepus::Value("text-content"));

  page->InsertNode(text);
  text->InsertNode(raw_text);

  text->SetAttribute("text-maxline", lepus::Value(1));

  page->FlushActionsAsRoot();
  ASSERT_NE(text->text_props_, nullptr);
  ASSERT_TRUE(text->text_props_->text_max_line.has_value());
  EXPECT_EQ(*text->text_props_->text_max_line, 1);

  text->ResetAttribute("text-maxline");
  text->FlushActionsAsRoot();

  ASSERT_NE(text->text_props_, nullptr);
  EXPECT_FALSE(text->text_props_->text_max_line.has_value());
}

TEST_P(TextElementTest, SameTextAttributesDoNotInvalidateLayoutInElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = EmbeddedMode::LAYOUT_IN_ELEMENT;
  auto page = manager->CreateFiberPage("page", 11);
  auto layout_text = manager->CreateFiberText("text");
  layout_text->SetAttribute("text", lepus::Value("text-content"));
  layout_text->SetAttribute("text-maxline", lepus::Value(1));
  page->InsertNode(layout_text);
  page->FlushActionsAsRoot();

  ASSERT_TRUE(layout_text->content_.IsEqual("text-content"));
  ASSERT_NE(layout_text->text_props_, nullptr);
  ASSERT_EQ(layout_text->text_props_->text_max_line, 1);
  layout_text->EnsureSLNode();
  layout_text->slnode()->is_dirty_ = false;

  layout_text->SetAttribute("text", lepus::Value("text-content"));
  layout_text->SetAttribute("text-maxline", lepus::Value(1));

  EXPECT_NE(layout_text->dirty() & Element::kDirtyAttr, 0);
  EXPECT_EQ(layout_text->updated_attr_map_.size(), 2u);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(layout_text->slnode()->IsDirty());
  EXPECT_TRUE(layout_text->updated_attr_map_.empty());

  layout_text->SetAttribute("text-maxline", lepus::Value(2));
  page->FlushActionsAsRoot();

  EXPECT_TRUE(layout_text->slnode()->IsDirty());
  EXPECT_EQ(layout_text->text_props_->text_max_line, 2);
}

TEST_P(TextElementTest, TestMeasureCase0) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  text->SetRawInlineStyles(base::String("color:red;"));
  text->SetAttribute("text-maxline", lepus::Value(1));

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_painting_node_ =
      painting_context->node_map_.at(text->impl_id()).get();

  EXPECT_EQ(mock_text_painting_node_->props_.size(), 1);
  EXPECT_EQ(text->text_props_->text_max_line, 1);
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDColor));
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->color, 4294901760);

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // color
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) == kTextPropColor);
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[1]) == -65536);

  // text max-line
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[2]) == kTextPropTextMaxLine);
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[3]) == 1);

  // text string
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[4]) == kPropTextString);
  EXPECT_TRUE(std::get<std::string>(mock_text_prop_array[5]) == "text-content");

  // inline-text
  auto inline_text = manager->CreateFiberText("text");
  //'测试' is 'testing', used for test the word's lenght in Chinese
  inline_text->SetAttribute("text", lepus::Value("-inline-测试"));
  inline_text->SetRawInlineStyles(base::String("color:blue;"));
  text->InsertNode(inline_text);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();

  int para_content_utf16_length = raw_text->content_utf16_length();
  int inline_content_utf16_length = inline_text->content_utf16_length();

  EXPECT_TRUE(para_content_utf16_length == 12);
  EXPECT_TRUE(inline_content_utf16_length == 10);

  auto& inline_mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;
  (void)inline_mock_text_prop_array;

  // kPropInlineStart
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[0]) ==
              kPropInlineStart);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[1]) ==
              para_content_utf16_length);

  // kTextPropColor
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[2]) == kTextPropColor);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[3]) ==
              -16776961);  // blue

  // kPropInlineEnd
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[4]) == kPropInlineEnd);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[5]) ==
              para_content_utf16_length + inline_content_utf16_length);

  //--- para
  // color
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[6]) == kTextPropColor);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[7]) == -65536);

  // text max-line
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[8]) ==
              kTextPropTextMaxLine);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[9]) == 1);

  // string
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[10]) ==
              kPropTextString);

  //'测试' is 'testing', used for test the word's lenght in Chinese
  EXPECT_TRUE(std::get<std::string>(inline_mock_text_prop_array[11]) ==
              "text-content-inline-测试");
}

TEST_P(TextElementTest, LayoutInElementFontScale) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  float mFontScale = 3.0f;
  // set font-scale:3
  manager->UpdateFontScale(mFontScale);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // color
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) == kTextPropFontSize);
  EXPECT_TRUE(std::get<double>(mock_text_prop_array[1]) ==
              manager->GetLynxEnvConfig().DefaultFontSize() * mFontScale);

  // text string
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[2]) == kPropTextString);
  EXPECT_TRUE(std::get<std::string>(mock_text_prop_array[3]) == "text-content");
}

INSTANTIATE_TEST_SUITE_P(TextElementTestModule, TextElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

TEST_P(TextElementTest, TextGradient) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  text->SetRawInlineStyles(
      base::String("color:linear-gradient(90deg, red, blue);"));

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // linear-gradient
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) ==
              kPropColorLinearGradient);
  // angle
  EXPECT_EQ(std::get<double>(mock_text_prop_array[1]), 90.0);

  // color_count
  EXPECT_EQ(std::get<int>(mock_text_prop_array[2]), 2);
  // Colors
  // color red
  EXPECT_EQ(std::get<int>(mock_text_prop_array[3]), -65536);
  // color blue
  EXPECT_EQ(std::get<int>(mock_text_prop_array[4]), -16776961);

  // position_count
  EXPECT_EQ(std::get<int>(mock_text_prop_array[5]), 0);

  // radial-gradient
  text->SetRawInlineStyles(
      base::String("color:radial-gradient(circle, red, blue);"));
  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();

  auto& mock_text_prop_array_radial =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  EXPECT_TRUE(std::get<int>(mock_text_prop_array_radial[0]) ==
              kPropColorRadialGradient);
  // check shape array

  // shape
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[1]), 1);
  // shape_size
  int shape_size = std::get<int>(mock_text_prop_array_radial[2]);
  EXPECT_EQ(shape_size, 0);
  // pos_x_pattern
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[3]),
            static_cast<int>(CSSValuePattern::PERCENT));
  // pos_x_value
  EXPECT_EQ(std::get<double>(mock_text_prop_array_radial[4]), 50.0f);
  // pos_y_pattern
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[5]),
            static_cast<int>(CSSValuePattern::PERCENT));
  // pos_y_value
  EXPECT_EQ(std::get<double>(mock_text_prop_array_radial[6]), 50.0f);
  // color_count
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[7]), 2);
  // colors
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[8]), -65536);
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[9]), -16776961);
  // positions
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[10]), 0);
}

TEST_P(TextElementTest, LayoutInElementWrapperTestCase0) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  text->SetRawInlineStyles(base::String("color:red"));

  page->InsertNode(text);

  auto wrapper = manager->CreateFiberWrapperElement();
  auto inline_text = manager->CreateFiberText("text");
  inline_text->SetRawInlineStyles(base::String("font-weight:500"));

  wrapper->InsertNode(inline_text);
  text->InsertNode(wrapper);

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);
  inline_text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // key0: {kPropInlineStart : 0}
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) == kPropInlineStart);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[1]), 0);

  // key1: {kTextPropFontWeight : FontWeightType::k500}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[2]), kTextPropFontWeight);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[3]),
            static_cast<int>(starlight::FontWeightType::k500));

  // key2: {kPropInlineEnd : raw_text->content_utf16_length_}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[4]), 1);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[5]),
            raw_text->content_utf16_length_);

  // key3: {kTextPropColor:-65536}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[6]), kTextPropColor);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[7]), -65536);

  // key4: {kPropTextString: "text-content"}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[8]), kPropTextString);
  EXPECT_EQ(std::get<std::string>(mock_text_prop_array[9]), "text-content");
}

TEST_P(TextElementTest, NewStylingLayoutInElementTextStylesReachTextMeasurer) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->enable_new_styling_pipeline_ = true;
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  auto inline_text = manager->CreateFiberText("text");
  inline_text->SetRawInlineStyles(
      base::String("font-size:26px;font-weight:500"));

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(inline_text);
  inline_text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  ASSERT_TRUE(inline_text->property_bits_.Has(kPropertyIDFontSize));
  ASSERT_TRUE(inline_text->property_bits_.Has(kPropertyIDFontWeight));

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());
  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;
  ASSERT_EQ(mock_text_prop_array.size(), 10u);

  EXPECT_EQ(std::get<int>(mock_text_prop_array[0]), kPropInlineStart);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[1]), 0);

  EXPECT_EQ(std::get<int>(mock_text_prop_array[2]), kTextPropFontSize);
  EXPECT_EQ(std::get<double>(mock_text_prop_array[3]), 26.0);

  EXPECT_EQ(std::get<int>(mock_text_prop_array[4]), kTextPropFontWeight);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[5]),
            static_cast<int>(starlight::FontWeightType::k500));

  EXPECT_EQ(std::get<int>(mock_text_prop_array[6]), kPropInlineEnd);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[7]),
            raw_text->content_utf16_length_);

  EXPECT_EQ(std::get<int>(mock_text_prop_array[8]), kPropTextString);
  EXPECT_EQ(std::get<std::string>(mock_text_prop_array[9]), "text-content");
}

TEST_P(TextElementTest, LayoutInElementWrapperTestCase1) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");

  auto wrapper = manager->CreateFiberWrapperElement();
  auto ref_inline_text = manager->CreateFiberText("text");  // empty

  auto inline_text = manager->CreateFiberText("text");
  inline_text->SetRawInlineStyles(base::String("color:red"));
  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);
  inline_text->InsertNode(raw_text);

  page->InsertNode(text);
  text->InsertNode(wrapper);
  text->InsertNode(ref_inline_text);

  wrapper->InsertNode(inline_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // key0: {kPropInlineStart : 0}
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) == kPropInlineStart);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[1]), 0);

  // key1: {kTextPropColor:-65536}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[2]), kTextPropColor);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[3]), -65536);

  // key2: {kPropInlineEnd : raw_text->content_utf16_length_}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[4]), 1);
  EXPECT_EQ(std::get<int>(mock_text_prop_array[5]),
            raw_text->content_utf16_length_);

  // key3: {kPropTextString: "text-content"}
  EXPECT_EQ(std::get<int>(mock_text_prop_array[6]), kPropTextString);
  EXPECT_EQ(std::get<std::string>(mock_text_prop_array[7]), "text-content");
}

TEST_P(TextElementTest, ResolveAndFlushFontFaces_AdoptedStyleSheet) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  // Create an adopted stylesheet with a font-face rule.
  CSSFontFaceRuleMap adopted_fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom-font.woff2)"}});
  adopted_fontfaces["custom-font"].push_back(font_rule);

  auto adopted_fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, CSSKeyframesTokenMap{},
      std::move(adopted_fontfaces));

  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  manager->AdoptStyleSheet(wrapper);

  // Verify the adopted sheet has font-face rules and is not yet resolved.
  ASSERT_FALSE(wrapper->fragment_->GetFontFaceRuleMap().empty());
  ASSERT_FALSE(wrapper->HasFontFacesResolved());

  // Call ResolveAndFlushFontFaces — it should also flush adopted font-faces.
  text->ResolveAndFlushFontFaces(base::String("custom-font"));

  // After flush, the wrapper should be marked as font-faces resolved.
  EXPECT_TRUE(wrapper->HasFontFacesResolved());
}

TEST_P(TextElementTest,
       ResolveAndFlushFontFaces_AdoptedStyleSheet_SkipIfAlreadyResolved) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  // Create an adopted stylesheet with a font-face rule, already resolved.
  CSSFontFaceRuleMap adopted_fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "another-font", CSSFontFaceAttrsMap{{"src", "url(another.woff2)"}});
  adopted_fontfaces["another-font"].push_back(font_rule);

  auto adopted_fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, CSSKeyframesTokenMap{},
      std::move(adopted_fontfaces));

  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  wrapper->MarkFontFacesResolved(true);
  manager->AdoptStyleSheet(wrapper);

  // Since font-faces are already resolved, second call should be a no-op.
  text->ResolveAndFlushFontFaces(base::String("another-font"));
  EXPECT_TRUE(wrapper->HasFontFacesResolved());
}

TEST_P(TextElementTest, NewStylingReplayFontFamilyFlushesFontFaces) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableNewStylingPipeline(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  CSSFontFaceRuleMap adopted_fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom-font.woff2)"}});
  adopted_fontfaces["custom-font"].push_back(font_rule);

  auto adopted_fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, CSSKeyframesTokenMap{},
      std::move(adopted_fontfaces));

  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  manager->AdoptStyleSheet(wrapper);

  ASSERT_FALSE(wrapper->HasFontFacesResolved());

  auto font_family = CSSValue::MakePlainString("custom-font");
  text->computed_css_style()->SetResolvedValue(kPropertyIDFontFamily,
                                               font_family);
  text->ReplayChangedStyleSideEffect(kPropertyIDFontFamily, font_family);

  EXPECT_TRUE(wrapper->HasFontFacesResolved());
}

TEST_P(TextElementTest, NewStylingResetFontFamilyDoesNotFlushFontFaces) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableNewStylingPipeline(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  CSSFontFaceRuleMap adopted_fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom-font.woff2)"}});
  adopted_fontfaces["custom-font"].push_back(font_rule);

  auto adopted_fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, CSSKeyframesTokenMap{},
      std::move(adopted_fontfaces));

  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  manager->AdoptStyleSheet(wrapper);

  ASSERT_FALSE(wrapper->HasFontFacesResolved());

  text->ReplayResetStyleSideEffect(kPropertyIDFontFamily);

  EXPECT_FALSE(wrapper->HasFontFacesResolved());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

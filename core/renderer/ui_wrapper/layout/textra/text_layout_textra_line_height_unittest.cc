// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <optional>

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_api.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

class TestTextElement : public TextElement {
 public:
  TestTextElement() : TextElement(nullptr, "text") {
    platform_css_style_ =
        std::make_unique<starlight::ComputedCSSStyle>(1.f, 1.);
  }
};

struct BackendState {
  int paragraph_token{0};
  std::optional<float> line_height;
};

class FakeParagraphBuilder : public text::ParagraphBuilder {
 public:
  explicit FakeParagraphBuilder(BackendState& state) : state_(state) {
    state_.line_height.reset();
  }

  void SetParagraphListener(text::ParagraphListener*) override {}
  void SetParagraphStyle(TextPropertyKeyID key, void* value,
                         size_t length) override {
    if (key == kTextPropLineHeight && length == sizeof(float)) {
      state_.line_height = *static_cast<float*>(value);
    }
  }
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
    return reinterpret_cast<text::Paragraph*>(&state_.paragraph_token);
  }

 private:
  BackendState& state_;
};

class FakeTextLayoutAPI : public text::TextLayoutAPI {
 public:
  explicit FakeTextLayoutAPI(BackendState& state) : state_(state) {}

  text::ParagraphBuilder* CreateParagraphBuilder() override {
    return new FakeParagraphBuilder(state_);
  }
  void DestroyParagraphBuilder(text::ParagraphBuilder* builder) override {
    delete builder;
  }
  text::MeasureResult MeasureParagraph(text::Paragraph*,
                                       text::MeasureParams) override {
    return {};
  }
  void AlignParagraph(text::Paragraph*, float, float) override {}
  text::Page* GetPage(text::Paragraph*) override { return nullptr; }
  void DestroyPage(text::Page*) override {}
  void DestroyParagraph(text::Paragraph*) override {}
  text::TextInfo GetTextInfo(std::string_view, float, std::string_view, float,
                             int) override {
    return {};
  }

 private:
  BackendState& state_;
};

}  // namespace

class TextLayoutTextraLineHeightTest : public ::testing::Test {
 public:
  void SetUp() override {
    layout_ = std::make_unique<TextLayoutTextra>(
        reinterpret_cast<intptr_t>(new FakeTextLayoutAPI(state_)));
  }

  void SetLineHeight(const char* value) {
    StyleMap styles;
    ASSERT_TRUE(UnitHandler::Process(kPropertyIDLineHeight, lepus::Value(value),
                                     styles, CSSParserConfigs()));
    element_.computed_css_style()->SetValue(kPropertyIDLineHeight,
                                            styles.at(kPropertyIDLineHeight));
    element_.property_bits().Set(kPropertyIDLineHeight);
  }

  BackendState state_;
  TestTextElement element_;
  std::unique_ptr<TextLayoutTextra> layout_;
};

TEST_F(TextLayoutTextraLineHeightTest, NormalUsesFontMetrics) {
  layout_->DispatchLayoutBefore(&element_);
  EXPECT_FALSE(state_.line_height.has_value());

  for (float font_size : {16.f, 0.5f}) {
    element_.computed_css_style()->SetFontSize(font_size, 16.f);
    SetLineHeight("normal");
    layout_->DispatchLayoutBefore(&element_);
    EXPECT_FALSE(state_.line_height.has_value());
  }
}

TEST_F(TextLayoutTextraLineHeightTest, UpdatesRestoreFontMetrics) {
  for (bool remove_property : {false, true}) {
    SetLineHeight("36px");
    layout_->DispatchLayoutBefore(&element_);
    ASSERT_TRUE(state_.line_height.has_value());
    EXPECT_FLOAT_EQ(*state_.line_height, 36.f);

    if (remove_property) {
      element_.computed_css_style()->ResetValue(kPropertyIDLineHeight);
    } else {
      SetLineHeight("normal");
    }
    // TextElement retains the property bit when a previously set style resets.
    layout_->DispatchLayoutBefore(&element_);
    EXPECT_FALSE(state_.line_height.has_value());
  }
}

TEST_F(TextLayoutTextraLineHeightTest, UndefinedBoundaryUsesFontMetrics) {
  element_.computed_css_style()->SetFontSize(16.f, 16.f);
  SetLineHeight("999999936px");
  layout_->DispatchLayoutBefore(&element_);
  ASSERT_TRUE(state_.line_height.has_value());
  EXPECT_FLOAT_EQ(*state_.line_height, 999999936.f);

  for (const auto& value : {"1000000000px", "1000000064px", "62500000"}) {
    SetLineHeight(value);
    layout_->DispatchLayoutBefore(&element_);
    EXPECT_FALSE(state_.line_height.has_value());
  }
}

TEST_F(TextLayoutTextraLineHeightTest, ExplicitHeightsRemainSupported) {
  element_.computed_css_style()->SetFontSize(16.f, 16.f);
  for (const auto& value : {"0", "0px", "1.5", "24px"}) {
    SetLineHeight(value);
    layout_->DispatchLayoutBefore(&element_);
    ASSERT_TRUE(state_.line_height.has_value());
    EXPECT_FLOAT_EQ(*state_.line_height, value[0] == '0' ? 0.f : 24.f);
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

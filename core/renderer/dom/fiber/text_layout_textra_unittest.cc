// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_api.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

constexpr auto kGradientKey = kTextPropTextGradient;

struct RecordedStyle {
  std::optional<text::TextGradient> gradient;
  uint32_t color{0};
};

struct RecordedRun {
  std::string text;
  RecordedStyle style;
};

// Storage outlives TextLayoutTextra, which owns and deletes the recording API.
struct Recording {
  std::vector<text::TextGradient> gradients;
  std::vector<RecordedRun> runs;
};

class RecordingParagraphBuilder : public text::ParagraphBuilder {
 public:
  explicit RecordingParagraphBuilder(Recording& recording)
      : recording_(recording) {}

  void SetParagraphListener(text::ParagraphListener*) override {}
  void SetParagraphStyle(TextPropertyKeyID, void*, size_t) override {}
  void PushTextStyle() override { styles_.push_back(style_); }
  void PopTextStyle() override {
    ASSERT_FALSE(styles_.empty());
    style_ = styles_.back();
    styles_.pop_back();
  }
  void PushEventTarget(const text::EventTargetInfo&) override {}
  void PopEventTarget() override {}
  void SetTextStyle(TextPropertyKeyID key, void* value,
                    size_t length) override {
    if (key == kGradientKey) {
      ASSERT_NE(value, nullptr);
      ASSERT_EQ(length, sizeof(text::TextGradient));
      // Copy synchronously; the caller's payload pointer is only borrowed.
      style_.gradient = *static_cast<text::TextGradient*>(value);
      recording_.gradients.push_back(*style_.gradient);
    } else if (key == kTextPropColor) {
      ASSERT_NE(value, nullptr);
      ASSERT_EQ(length, sizeof(int));
      style_.color = static_cast<uint32_t>(*static_cast<int*>(value));
      style_.gradient.reset();
    }
  }
  void AddText(const char* text, size_t length) override {
    recording_.runs.push_back({std::string(text, length), style_});
  }
  void AddInlineView(std::unique_ptr<text::InlineView>) override {}
  void AddImage(const char*, size_t) override {}
  void StartInlineTruncation() override {}
  void EndInlineTruncation() override {}
  void SetPlaceHolderStyle(TextPropertyKeyID, void*, size_t) override {}
  text::Paragraph* BuildParagraph() override { return nullptr; }

 private:
  Recording& recording_;
  RecordedStyle style_;
  std::vector<RecordedStyle> styles_;
};

class RecordingTextLayoutAPI : public text::TextLayoutAPI {
 public:
  explicit RecordingTextLayoutAPI(Recording& recording)
      : recording_(recording) {}
  text::ParagraphBuilder* CreateParagraphBuilder() override {
    return new RecordingParagraphBuilder(recording_);
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
  Recording& recording_;
};

void ExpectGradient(const text::TextGradient& actual,
                    const text::TextGradient& expected) {
  EXPECT_EQ(actual.type, expected.type);
  EXPECT_EQ(actual.geometry, expected.geometry);
  EXPECT_EQ(actual.colors, expected.colors);
  EXPECT_EQ(actual.stops, expected.stops);
}

text::TextGradient LinearGradient() {
  return {starlight::BackgroundImageType::kLinearGradient,
          {90, static_cast<float>(starlight::LinearGradientDirection::kAngle)},
          {0xffff0000u, 0xff0000ffu},
          {10, 90}};
}

void RecordLayout(TextElement* element, Recording& recording) {
  TextLayoutTextra layout(
      reinterpret_cast<intptr_t>(new RecordingTextLayoutAPI(recording)));
  layout.DispatchLayoutBefore(element);
}

}  // namespace

class TextLayoutTextraTest : public FiberElementTest {
 public:
  void SetUp() override {
    FiberElementTest::SetUp();
    auto options = manager->GetPageOptions();
    options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
    manager->SetPageOptions(options);
  }

 protected:
  void AddText(Element* parent, const char* content) {
    auto raw = manager->CreateFiberRawText();
    raw->SetText(lepus::Value(content));
    parent->InsertNode(raw);
  }

  void CheckGradient(const char* css, starlight::BackgroundImageType type) {
    auto page = manager->CreateFiberPage("page", 11);
    auto text = manager->CreateFiberText("text");
    text->SetRawInlineStyles(base::String(css));
    AddText(text.get(), "gradient");
    page->InsertNode(text);
    page->FlushActionsAsRoot();

    const auto& attributes = text->computed_css_style()->GetTextAttributes();
    ASSERT_TRUE(attributes.has_value());
    ASSERT_TRUE(attributes->text_gradient.has_value());
    const auto original = lepus::Value::Clone(*attributes->text_gradient);
    ASSERT_TRUE(original.IsArray());
    auto expected = LinearGradient();
    if (type == starlight::BackgroundImageType::kRadialGradient) {
      expected.type = type;
      expected.geometry = {
          static_cast<float>(starlight::RadialGradientShapeType::kCircle),
          static_cast<float>(starlight::RadialGradientSizeType::kLength),
          static_cast<float>(CSSValuePattern::PERCENT),
          50,
          static_cast<float>(CSSValuePattern::PERCENT),
          50,
          0,
          0,
          0,
          0,
          10,
          static_cast<float>(starlight::PlatformLengthUnit::NUMBER),
          10,
          static_cast<float>(starlight::PlatformLengthUnit::NUMBER)};
      // The computed style retains the unresolved CSS length payload.
      const auto shape = original.Array()->get(1).Array()->get(0).Array();
      ASSERT_GE(shape->size(), 10u);
      EXPECT_EQ(shape->get(6).Number(), static_cast<int>(CSSValuePattern::PX));
      EXPECT_EQ(shape->get(7).Number(), 10);
      EXPECT_EQ(shape->get(8).Number(), static_cast<int>(CSSValuePattern::PX));
      EXPECT_EQ(shape->get(9).Number(), 10);
    }

    Recording recording;
    RecordLayout(text.get(), recording);
    // Layout must not mutate the original CSS data, including source lengths.
    EXPECT_EQ(*attributes->text_gradient, original);
    ASSERT_EQ(recording.gradients.size(), 1u);
    ExpectGradient(recording.gradients.front(), expected);
    ASSERT_EQ(recording.runs.size(), 1u);
    EXPECT_EQ(recording.runs.front().text, "gradient");
    ASSERT_TRUE(recording.runs.front().style.gradient.has_value());
    ExpectGradient(*recording.runs.front().style.gradient, expected);

    // A subsequent solid color must not reuse the preceding gradient.
    text->SetRawInlineStyles(base::String("color:green;"));
    page->FlushActionsAsRoot();
    Recording solid;
    RecordLayout(text.get(), solid);
    EXPECT_TRUE(solid.gradients.empty());
    ASSERT_EQ(solid.runs.size(), 1u);
    EXPECT_FALSE(solid.runs.front().style.gradient.has_value());
    EXPECT_EQ(solid.runs.front().style.color, 0xff008000u);
    ExpectGradient(recording.gradients.front(), expected);
    ExpectGradient(*recording.runs.front().style.gradient, expected);
  }
};

TEST_P(TextLayoutTextraTest, EmitsLinearGradientWithColorsAndStops) {
  CheckGradient("color:linear-gradient(90deg, red 10%, blue 90%);",
                starlight::BackgroundImageType::kLinearGradient);
}

TEST_P(TextLayoutTextraTest, EmitsRadialGradientWithColorsAndStops) {
  CheckGradient("color:radial-gradient(circle 10px, red 10%, blue 90%);",
                starlight::BackgroundImageType::kRadialGradient);
}

TEST_P(TextLayoutTextraTest, NestedSolidColorOverridesAndRestoresGradient) {
  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  text->SetRawInlineStyles(
      base::String("color:linear-gradient(90deg, red 10%, blue 90%);"));
  page->InsertNode(text);

  auto inherited = manager->CreateFiberText("text");
  AddText(inherited.get(), "inherited");
  text->InsertNode(inherited);
  auto solid = manager->CreateFiberText("text");
  solid->SetRawInlineStyles(base::String("color:green;"));
  AddText(solid.get(), "solid");
  text->InsertNode(solid);
  AddText(text.get(), "restored");
  page->FlushActionsAsRoot();

  const auto& attributes = text->computed_css_style()->GetTextAttributes();
  ASSERT_TRUE(attributes.has_value());
  ASSERT_TRUE(attributes->text_gradient.has_value());
  const auto expected = LinearGradient();

  Recording recording;
  RecordLayout(text.get(), recording);
  ASSERT_EQ(recording.runs.size(), 3u);
  EXPECT_EQ(recording.runs[0].text, "inherited");
  ASSERT_TRUE(recording.runs[0].style.gradient.has_value());
  ExpectGradient(*recording.runs[0].style.gradient, expected);
  EXPECT_EQ(recording.runs[1].text, "solid");
  EXPECT_FALSE(recording.runs[1].style.gradient.has_value());
  EXPECT_EQ(recording.runs[1].style.color, 0xff008000u);
  EXPECT_EQ(recording.runs[2].text, "restored");
  ASSERT_TRUE(recording.runs[2].style.gradient.has_value());
  ExpectGradient(*recording.runs[2].style.gradient, expected);
}

INSTANTIATE_TEST_SUITE_P(TextLayoutTextraTestModule, TextLayoutTextraTest,
                         ::testing::Values(std::make_tuple(0, 0)));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "base/include/fml/thread.h"
#include "clay/fml/icu_util.h"
#include "clay/fml/paths.h"
#include "clay/third_party/txt/src/txt/font_collection.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/resource/font_collection.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/modules/skparagraph/include/TypefaceFontProvider.h"

namespace clay {

namespace {
constexpr float kEnoughSpace = 1000.f;
constexpr float kInsufficientWidth = 100.f;
constexpr float kInsufficientHeight = 10.f;

constexpr float kHelloWorldMaxIntrinsicWidth = 247.f;
constexpr float kHelloWorldNoWrapHeight = 59.f;
constexpr float kHelloWorldSoftWrapHeight = kHelloWorldNoWrapHeight * 4;

const char* kTestTypefacePath =
    "gen/lynx/clay/third_party/txt/assets/Roboto-Bold.ttf";
const char* kTestFontFamily = "Roboto";

class TextViewTest : public ::testing::Test {
 protected:
  TextViewTest() : thread_("ui") {
    fml::icu::InitializeICU("icudtl.dat");
    InitializeFontCollection();
  }

  ~TextViewTest() = default;

  void InitializeFontCollection() {
    auto font_collection = clay::FontCollection::Instance();
    font_collection->SetupDefaultFontManager(0);
    auto txt_font_collection = font_collection->GetFontCollection();
    txt_font_collection->SetAssetFontManager(CreateTestFontProvider());
    txt_font_collection->DisableFontFallback();
    auto skt_fc = txt_font_collection->CreateSktFontCollection();
  }

  sk_sp<skia::textlayout::TypefaceFontProvider> CreateTestFontProvider() {
    auto font_provider = sk_make_sp<skia::textlayout::TypefaceFontProvider>();
    auto directory = fml::paths::GetExecutableDirectoryPath();
    if (directory.first) {
      auto ttf_path =
          fml::paths::JoinPaths({directory.second, kTestTypefacePath});
      font_provider->registerTypeface(
          SkTypeface::MakeFromFile(ttf_path.c_str()));
    } else {
      FML_LOG(ERROR) << "Failed get test font ttf files.";
    }
    return font_provider;
  }

  void SetUp() override {
    page_view_.reset(new PageView(-1, nullptr, thread_.GetTaskRunner()));
    text_view_.reset(new TextView(-1, page_view_.get()));
    text_view_->SetFontWeight(FontWeight::kBold);
    text_view_->SetFontSize(50);
    text_view_->SetPaddings(0.f, 0.f, 0.f, 0.f);
    text_view_->SetFontFamily(kTestFontFamily);
  }

 protected:
  void SetupTestText(const char* content) {
    auto raw_text = std::make_unique<RawTextView>(-1, nullptr);
    raw_text->SetText(content);
    text_view_->AddChild(raw_text.get());
    holder_.emplace_back(std::move(raw_text));
  }

  void SetupHelloWorldText() { SetupTestText("hello world"); }

  void SetupMultilinesText() { SetupTestText("hello world\nhello world"); }

  // For lifecycle management.
  std::vector<std::unique_ptr<BaseView>> holder_;
  std::unique_ptr<TextView> text_view_;
  std::unique_ptr<PageView> page_view_;
  fml::Thread thread_;
};
}  // namespace

TEST_F(TextViewTest, MeasureWithEnoughSpaceTest) {
  SetupHelloWorldText();
  MeasureResult result;

  text_view_->Measure({kEnoughSpace, MeasureMode::kDefinite, kEnoughSpace,
                       MeasureMode::kDefinite},
                      result);
  EXPECT_EQ(result.width, kEnoughSpace);
  EXPECT_EQ(result.height, kEnoughSpace);

  text_view_->Measure(
      {kEnoughSpace, MeasureMode::kAtMost, kEnoughSpace, MeasureMode::kAtMost},
      result);
  EXPECT_EQ(result.width, kHelloWorldMaxIntrinsicWidth);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight);

  text_view_->Measure(
      {0.f, MeasureMode::kIndefinite, 0.f, MeasureMode::kIndefinite}, result);
  EXPECT_EQ(result.width, kHelloWorldMaxIntrinsicWidth);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight);
}

TEST_F(TextViewTest, MeasureWithoutEnoughSpaceTest) {
  SetupHelloWorldText();
  MeasureResult result;

  text_view_->Measure({kInsufficientWidth, MeasureMode::kDefinite,
                       kInsufficientHeight, MeasureMode::kDefinite},
                      result);
  EXPECT_EQ(result.width, kInsufficientWidth);
  EXPECT_EQ(result.height, kInsufficientHeight);

  // will wrap into 3 lines.
  text_view_->Measure(
      {kInsufficientWidth, MeasureMode::kAtMost, 0.f, MeasureMode::kIndefinite},
      result);
  EXPECT_EQ(result.width, kInsufficientWidth);
  EXPECT_EQ(result.height, kHelloWorldSoftWrapHeight);
}

TEST_F(TextViewTest, MeasureLimitedLinesTest) {
  text_view_->SetTextMaxLine(1);

  SetupHelloWorldText();
  MeasureResult result;

  // No wrap. It will be clipped.
  text_view_->Measure({kInsufficientWidth, MeasureMode::kDefinite, 0.f,
                       MeasureMode::kIndefinite},
                      result);
  EXPECT_EQ(result.width, kInsufficientWidth);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight);

  text_view_->SetTextMaxLine(2);

  text_view_->Measure({kInsufficientWidth, MeasureMode::kDefinite, 0.f,
                       MeasureMode::kIndefinite},
                      result);
  EXPECT_EQ(result.width, kInsufficientWidth);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight * 2);
}

TEST_F(TextViewTest, MeasureMultiLineContentTest) {
  SetupMultilinesText();
  MeasureResult result;

  // No wrap. It will be clipped.
  text_view_->Measure(
      {0.f, MeasureMode::kIndefinite, 0.f, MeasureMode::kIndefinite}, result);
  EXPECT_EQ(result.width, kHelloWorldMaxIntrinsicWidth);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight * 2);

  text_view_->SetTextMaxLine(1);

  // Ignore second line.
  text_view_->Measure(
      {0.f, MeasureMode::kIndefinite, 0.f, MeasureMode::kIndefinite}, result);
  EXPECT_EQ(result.width, kHelloWorldMaxIntrinsicWidth);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight);
}

TEST_F(TextViewTest, MeasureWithNoContentTest) {
  MeasureResult result;

  // No content is equal to has content with enough space.
  text_view_->Measure({kEnoughSpace, MeasureMode::kIndefinite, kEnoughSpace,
                       MeasureMode::kIndefinite},
                      result);
  EXPECT_EQ(result.width, 0.f);
  EXPECT_EQ(result.height, kHelloWorldNoWrapHeight);
}

}  // namespace clay

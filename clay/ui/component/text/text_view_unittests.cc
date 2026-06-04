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
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_shadow_node.h"
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
    shadow_owner_ = std::make_unique<ShadowNodeOwner>(thread_.GetTaskRunner());
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

  TextShadowNode* SetupEditableTextShadow() {
    shadow_node_holder_.clear();
    editable_text_shadow_ =
        std::make_unique<TextShadowNode>(shadow_owner_.get(), "text", -1);
    text_view_->SetEditableTextShadowNodeForTesting(
        editable_text_shadow_.get());
    return editable_text_shadow_.get();
  }

  template <typename Node>
  Node* MakeShadowNode(const std::string& tag) {
    auto node = std::make_unique<Node>(shadow_owner_.get(), tag, -1);
    auto* ptr = node.get();
    shadow_node_holder_.emplace_back(std::move(node));
    return ptr;
  }

  RawTextShadowNode* AddRawText(BaseTextShadowNode* parent,
                                const std::u16string& text) {
    auto* raw_text = MakeShadowNode<RawTextShadowNode>("raw-text");
    raw_text->SetText(text);
    parent->AddChild(raw_text);
    return raw_text;
  }

  // For lifecycle management.
  std::vector<std::unique_ptr<BaseView>> holder_;
  std::vector<std::unique_ptr<ShadowNode>> shadow_node_holder_;
  std::unique_ptr<TextView> text_view_;
  std::unique_ptr<PageView> page_view_;
  std::unique_ptr<ShadowNodeOwner> shadow_owner_;
  std::unique_ptr<TextShadowNode> editable_text_shadow_;
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

TEST_F(TextViewTest, NonContenteditableKeyEventKeepsOriginalHandledResult) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");

  KeyEvent key_event(0, KeyEventType::kDown, 0, 0, false, "!");
  EXPECT_FALSE(text_view_->OnKeyEvent(&key_event));

  EXPECT_FALSE(text_view_->IsContentEditable());
  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi"));
}

TEST_F(TextViewTest, ContenteditableKeyEventInsertsText) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  KeyEvent key_event(0, KeyEventType::kDown, 0, 0, true, "!");
  EXPECT_TRUE(text_view_->OnKeyEvent(&key_event));

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi!"));
  EXPECT_EQ(text_view_->GetRenderText()->GetText(), std::u16string(u"hi!"));
}

TEST_F(TextViewTest, ShadowContenteditableKeyEventInsertsText) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");
  text_shadow->SetAttribute("contenteditable", clay::Value(true));

  KeyEvent key_event(0, KeyEventType::kDown, 0, 0, true, "!");
  EXPECT_TRUE(text_view_->OnKeyEvent(&key_event));

  EXPECT_FALSE(text_view_->IsContentEditable());
  EXPECT_TRUE(text_shadow->IsContentEditable());
  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi!"));
  EXPECT_EQ(text_view_->GetRenderText()->GetText(), std::u16string(u"hi!"));
}

TEST_F(TextViewTest, ContenteditableDisabledDoesNotInsertText) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");

  EXPECT_FALSE(text_view_->InsertEditableText(u"!"));

  EXPECT_FALSE(text_view_->IsContentEditable());
  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi"));
}

TEST_F(TextViewTest,
       ContenteditableInsertWithoutViewContextCreatesRawTextAndUpdatesRenderText) {
  auto* text_shadow = SetupEditableTextShadow();
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  EXPECT_EQ(page_view_->GetViewContext(), nullptr);
  EXPECT_TRUE(text_view_->InsertEditableText(u"a"));

  auto stream = text_shadow->BuildEditableInlineStream();
  ASSERT_EQ(stream.size(), 1u);
  EXPECT_EQ(stream[0].type, EditableInlineStreamItemType::kText);
  EXPECT_EQ(stream[0].text, std::u16string(u"a"));
  EXPECT_EQ(text_view_->GetRenderText()->GetText(), std::u16string(u"a"));
}

TEST_F(TextViewTest, ContenteditableCommitTextPreservesUtf16Emoji) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"A");
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  text_view_->OnCommitText("\xF0\x9F\x98\x80");

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"A\U0001F600"));
  EXPECT_EQ(raw_text->EditableText().length(), 3u);
}

TEST_F(TextViewTest, ContenteditableNativeKeyEventDoesNotBypassIme) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  KeyEvent key_event(0, KeyEventType::kDown, 0, 0, false, "!");
  EXPECT_FALSE(text_view_->OnKeyEvent(&key_event));

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi"));
}

TEST_F(TextViewTest, ContenteditableComposingTextDoesNotCommitSource) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  text_view_->OnComposingText("zhong");

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi"));
}

TEST_F(TextViewTest, ContenteditablePlatformComposingStateDoesNotCommitSource) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"hi");
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  text_view_->UpdateEditingState("hizhong",
                                 TextSelection(7, 7, Affinity::kDownstream),
                                 TextRange(2, 7), Affinity::kDownstream);

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"hi"));
  EXPECT_EQ(text_view_->GetRenderText()->GetText(), std::u16string(u"hi"));
}

TEST_F(TextViewTest, ContenteditableInsertUsesCaretOffset) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"abcd");
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->SetEditableCaretOffsetForTesting(2);

  EXPECT_TRUE(text_view_->InsertEditableText(u"X"));

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"abXcd"));
}

TEST_F(TextViewTest, ContenteditableBackspaceDeletesWholeEmoji) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"A\U0001F600B");
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->SetEditableCaretOffsetForTesting(3);

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"AB"));
}

TEST_F(TextViewTest, ContenteditableBackspaceDeletesSelectionRange) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"abcd");
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->GetRenderText()->SetSelection(TextRange(1, 3));

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"ad"));
  EXPECT_TRUE(text_view_->InsertEditableText(u"X"));
  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"aXd"));
}

TEST_F(TextViewTest, ContenteditableUpdateEditingStateAppliesInlineDiff) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* raw_text = AddRawText(text_shadow, u"ab");
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  text_view_->UpdateEditingState("aXb",
                                 TextSelection(2, 2, Affinity::kDownstream),
                                 TextRange(0), Affinity::kDownstream);

  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"aXb"));
}

TEST_F(TextViewTest, ContenteditableInlineTextParticipatesInInsertion) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* leading_raw_text = AddRawText(text_shadow, u"A");
  auto* inline_text = MakeShadowNode<InlineTextShadowNode>("inline-text");
  auto* inline_raw_text = AddRawText(inline_text, u"B");
  text_shadow->AddChild(inline_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  EXPECT_TRUE(text_view_->InsertEditableText(u"C"));

  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"A"));
  EXPECT_EQ(inline_raw_text->EditableText(), std::u16string(u"BC"));
  EXPECT_EQ(text_shadow->BuildEditableInlineStream().size(), 2u);
}

TEST_F(TextViewTest, ContenteditableDeleteInlineTextRangeKeepsEditing) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* leading_raw_text = AddRawText(text_shadow, u"A");
  auto* inline_text = MakeShadowNode<InlineTextShadowNode>("inline-text");
  auto* inline_raw_text = AddRawText(inline_text, u"BC");
  auto* trailing_raw_text = AddRawText(text_shadow, u"D");
  text_shadow->AddChild(inline_text);
  text_shadow->RemoveChild(trailing_raw_text);
  text_shadow->AddChild(trailing_raw_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->GetRenderText()->SetSelection(TextRange(1, 3));

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));
  EXPECT_EQ(inline_raw_text->EditableText(), std::u16string());

  EXPECT_TRUE(text_view_->InsertEditableText(u"X"));
  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"AX"));
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"D"));
}

TEST_F(TextViewTest, ContenteditableBackspaceDeletesInlineImageToken) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* leading_raw_text = AddRawText(text_shadow, u"A");
  auto* inline_image = MakeShadowNode<InlineImageShadowNode>("inline-image");
  inline_image->EnsureDefaultStyle();
  auto* trailing_raw_text = AddRawText(text_shadow, u"B");
  text_shadow->AddChild(inline_image);
  text_shadow->RemoveChild(trailing_raw_text);
  text_shadow->AddChild(trailing_raw_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->SetEditableCaretOffsetForTesting(2);

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));

  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"A"));
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"B"));
  auto stream = text_shadow->BuildEditableInlineStream();
  ASSERT_EQ(stream.size(), 2u);
  EXPECT_EQ(stream[0].type, EditableInlineStreamItemType::kText);
  EXPECT_EQ(stream[1].type, EditableInlineStreamItemType::kText);
  EXPECT_EQ(inline_image->Parent(), nullptr);
}

TEST_F(TextViewTest, ContenteditableRangeDeleteRemovesInlineViewToken) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* leading_raw_text = AddRawText(text_shadow, u"A");
  auto* inline_view = MakeShadowNode<InlineViewShadowNode>("inline-view");
  inline_view->EnsureDefaultStyle();
  auto* trailing_raw_text = AddRawText(text_shadow, u"B");
  text_shadow->AddChild(inline_view);
  text_shadow->RemoveChild(trailing_raw_text);
  text_shadow->AddChild(trailing_raw_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->GetRenderText()->SetSelection(TextRange(1, 2));

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));

  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"A"));
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"B"));
  EXPECT_EQ(inline_view->Parent(), nullptr);
}

TEST_F(TextViewTest, ContenteditableInsertionAtAtomicBoundaryKeepsAtomicToken) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* leading_raw_text = AddRawText(text_shadow, u"A");
  auto* inline_image = MakeShadowNode<InlineImageShadowNode>("inline-image");
  inline_image->EnsureDefaultStyle();
  auto* trailing_raw_text = AddRawText(text_shadow, u"B");
  text_shadow->AddChild(inline_image);
  text_shadow->RemoveChild(trailing_raw_text);
  text_shadow->AddChild(trailing_raw_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->SetEditableCaretOffsetForTesting(1);

  EXPECT_TRUE(text_view_->InsertEditableText(u"X"));

  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"AX"));
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"B"));
  auto stream = text_shadow->BuildEditableInlineStream();
  ASSERT_EQ(stream.size(), 3u);
  EXPECT_EQ(stream[1].type,
            EditableInlineStreamItemType::kAtomicInlineImage);
  EXPECT_EQ(stream[1].node, inline_image);
}

TEST_F(TextViewTest, ContenteditableCommandASelectsWholeAtomicInlineStream) {
  auto* text_shadow = SetupEditableTextShadow();
  AddRawText(text_shadow, u"A");
  auto* inline_view = MakeShadowNode<InlineViewShadowNode>("inline-view");
  inline_view->EnsureDefaultStyle();
  auto* trailing_raw_text = AddRawText(text_shadow, u"B");
  text_shadow->AddChild(inline_view);
  text_shadow->RemoveChild(trailing_raw_text);
  text_shadow->AddChild(trailing_raw_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));

  KeyEvent meta_down(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kMeta), false, "");
  KeyEvent a_down(0, KeyEventType::kDown, 0,
                  static_cast<uint64_t>(KeyCode::kKeyA), false, "a");
  text_view_->OnKeyEvent(&meta_down);
  text_view_->OnKeyEvent(&a_down);

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));

  EXPECT_TRUE(text_shadow->BuildEditableInlineStream().empty());
  EXPECT_EQ(inline_view->Parent(), nullptr);
}

TEST_F(TextViewTest, ContenteditableUndoRestoresDeletedInlineViewToken) {
  auto* text_shadow = SetupEditableTextShadow();
  auto* leading_raw_text = AddRawText(text_shadow, u"A");
  auto* inline_view = MakeShadowNode<InlineViewShadowNode>("inline-view");
  inline_view->EnsureDefaultStyle();
  auto* trailing_raw_text = AddRawText(text_shadow, u"B");
  text_shadow->AddChild(inline_view);
  text_shadow->RemoveChild(trailing_raw_text);
  text_shadow->AddChild(trailing_raw_text);
  text_view_->SetAttribute("contenteditable", clay::Value(true));
  text_view_->SetEditableCaretOffsetForTesting(2);

  KeyEvent backspace(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kBackspace), true, "");
  EXPECT_TRUE(text_view_->OnKeyEvent(&backspace));
  EXPECT_EQ(inline_view->Parent(), nullptr);

  KeyEvent meta_down(0, KeyEventType::kDown, 0,
                     static_cast<uint64_t>(KeyCode::kMeta), false, "");
  KeyEvent z_down(0, KeyEventType::kDown, 0,
                  static_cast<uint64_t>(KeyCode::kKeyZ), false, "z");
  text_view_->OnKeyEvent(&meta_down);
  text_view_->OnKeyEvent(&z_down);

  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"A"));
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"B"));
  EXPECT_EQ(inline_view->Parent(), text_shadow);
  auto stream = text_shadow->BuildEditableInlineStream();
  ASSERT_EQ(stream.size(), 3u);
  EXPECT_EQ(stream[1].type, EditableInlineStreamItemType::kAtomicInlineView);
  EXPECT_EQ(stream[1].node, inline_view);
}

}  // namespace clay

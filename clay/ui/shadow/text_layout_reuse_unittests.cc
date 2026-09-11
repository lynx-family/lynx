// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_render.h"
#include "clay/ui/shadow/text_shadow_node.h"
#include "clay/ui/testing/ui_test.h"
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
#include "clay/ui/testing/text_layout_reuse_test_hooks.h"
#endif

namespace clay {

class TextRenderReuseTestPeer {
 public:
  static const char* Reject(const TextRender& render, double width) {
    return render.GetAtMostShrinkReuseRejectionReason(width);
  }
  static TextRender& Render(TextShadowNode& node) { return *node.text_render_; }
  static float RequestedWidth(const TextRender& render) {
    return render.prev_layout_width_;
  }
};

class TextLayoutReuseTest : public UITest {
 protected:
  void UISetUp() override {
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
    // Do not depend on the process-wide rollout setting. Individual tests
    // enable the real Measure branch explicitly, even when rollout is off.
    hooks_ = std::make_unique<ScopedTextLayoutReuseTestHooks>();
#endif
    owner_ = std::make_unique<ShadowNodeOwner>(
        fml::MessageLoop::GetCurrent().GetTaskRunner());
    view_context_ = std::make_unique<ViewContext>(page_.get(), owner_.get());
    owner_->SetViewContext(view_context_.get());
    node_ = std::make_unique<TextShadowNode>(owner_.get(), "text", 1);
    raw_ = std::make_unique<RawTextShadowNode>(owner_.get(), "raw-text", 2);
    node_->AddChild(raw_.get());
    node_->SetFontSize(20.f);
    raw_->SetText("Hello world");
  }

  void UITearDown() override {
    node_.reset();
    raw_.reset();
    extra_raw_.clear();
    owner_->SetViewContext(nullptr);
    view_context_.reset();
    owner_.reset();
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
    hooks_.reset();
#endif
  }

  MeasureResult Measure(float width, MeasureMode mode = MeasureMode::kAtMost) {
    return node_->Measure(
        {width, mode, std::nullopt, MeasureMode::kIndefinite});
  }
  // Exercise the first-layout cache and reuse policy without the rollout gate
  // or an automatic second layout. Production activation is unchanged.
  ShadowLayoutContextMeasure BuildFirstLayout(float width) {
    const MeasureConstraint constraint = {
        width, MeasureMode::kAtMost, std::nullopt, MeasureMode::kIndefinite};
    node_->CreateTextBundle();
    node_->ResetEndIndex();
    auto context = node_->CreateLayoutContext(constraint);
    Render().BuildTextLayout(constraint, &context);
    return context;
  }
  TextRender& Render() { return TextRenderReuseTestPeer::Render(*node_); }
  const char* Reject(double width = 1000.0) {
    return TextRenderReuseTestPeer::Reject(Render(), width);
  }

  RawTextShadowNode* AddRawText(const std::u16string& text) {
    auto raw = std::make_unique<RawTextShadowNode>(owner_.get(), "raw-text",
                                                   3 + extra_raw_.size());
    raw->SetText(text);
    auto* result = raw.get();
    extra_raw_.push_back(std::move(raw));
    node_->AddChild(result);
    return result;
  }

  static void ExpectSameGeometry(txt::Paragraph& original,
                                 txt::Paragraph& rebuilt, size_t text_size) {
    EXPECT_DOUBLE_EQ(original.GetHeight(), rebuilt.GetHeight());
    EXPECT_DOUBLE_EQ(original.GetMaxIntrinsicWidth(),
                     rebuilt.GetMaxIntrinsicWidth());
    EXPECT_DOUBLE_EQ(original.GetAlphabeticBaseline(),
                     rebuilt.GetAlphabeticBaseline());
    EXPECT_DOUBLE_EQ(original.GetIdeographicBaseline(),
                     rebuilt.GetIdeographicBaseline());
    const auto& before = original.GetLineMetrics();
    const auto& after = rebuilt.GetLineMetrics();
    ASSERT_EQ(before.size(), 1u);
    ASSERT_EQ(after.size(), 1u);
    EXPECT_EQ(before[0].start_index, after[0].start_index);
    EXPECT_EQ(before[0].end_index, after[0].end_index);
    EXPECT_EQ(before[0].hard_break, after[0].hard_break);
    EXPECT_DOUBLE_EQ(before[0].width, after[0].width);
    EXPECT_DOUBLE_EQ(before[0].height, after[0].height);
    EXPECT_DOUBLE_EQ(before[0].left, after[0].left);
    EXPECT_DOUBLE_EQ(before[0].baseline, after[0].baseline);
    EXPECT_DOUBLE_EQ(before[0].ascent, after[0].ascent);
    EXPECT_DOUBLE_EQ(before[0].descent, after[0].descent);
    for (size_t i = 0; i < text_size; ++i) {
      // Cover both individual characters and selections crossing raw nodes.
      for (size_t end : {i + 1, text_size}) {
        const auto a = original.GetRectsForRange(
            i, end, txt::Paragraph::RectHeightStyle::kTight,
            txt::Paragraph::RectWidthStyle::kTight);
        const auto b = rebuilt.GetRectsForRange(
            i, end, txt::Paragraph::RectHeightStyle::kTight,
            txt::Paragraph::RectWidthStyle::kTight);
        ASSERT_EQ(a.size(), b.size());
        for (size_t j = 0; j < a.size(); ++j) {
          EXPECT_EQ(a[j].direction, b[j].direction);
          EXPECT_FLOAT_EQ(a[j].rect.Left(), b[j].rect.Left());
          EXPECT_FLOAT_EQ(a[j].rect.Right(), b[j].rect.Right());
          EXPECT_FLOAT_EQ(a[j].rect.Top(), b[j].rect.Top());
          EXPECT_FLOAT_EQ(a[j].rect.Bottom(), b[j].rect.Bottom());
        }
      }
    }
    for (double y :
         {0.0, original.GetAlphabeticBaseline(), original.GetHeight()}) {
      for (double x = -1; x <= std::ceil(original.GetMaxIntrinsicWidth()) + 1;
           x += 0.5) {
        const auto a = original.GetGlyphPositionAtCoordinate(x, y);
        const auto b = rebuilt.GetGlyphPositionAtCoordinate(x, y);
        EXPECT_EQ(a.position, b.position);
        EXPECT_EQ(a.affinity, b.affinity);
      }
    }
  }

  std::unique_ptr<ShadowNodeOwner> owner_;
  std::unique_ptr<ViewContext> view_context_;
  std::unique_ptr<TextShadowNode> node_;
  std::unique_ptr<RawTextShadowNode> raw_;
  std::vector<std::unique_ptr<RawTextShadowNode>> extra_raw_;
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  std::unique_ptr<ScopedTextLayoutReuseTestHooks> hooks_;
#endif
};

TEST_F_UI(TextLayoutReuseTest, RawTextViewPreservesTruncation) {
  const auto& raw_node = *raw_;
  for (const auto& text :
       {std::u16string(), std::u16string(u"plain text"),
        std::u16string(u"\u4e2d\u6587"), std::u16string(u"\U0001f600"),
        std::u16string(u"a\0b", 3),
        std::u16string(1, static_cast<char16_t>(0xD800)),
        std::u16string(4096, u'x')}) {
    raw_->SetText(text);
    raw_->SetEndIndex(std::numeric_limits<size_t>::max());
    const auto* data = raw_node.GetTruncatedTextView().data();
    for (size_t end : {size_t{0}, size_t{1}, size_t{2}, text.size(),
                       text.size() + 1, std::numeric_limits<size_t>::max()}) {
      raw_->SetEndIndex(end);
      const auto view = raw_node.GetTruncatedTextView();
      EXPECT_EQ(std::u16string(view), raw_->Text());
      EXPECT_EQ(view.data(), data);
    }
  }
}

TEST_F_UI(TextLayoutReuseTest, RawTextViewReflectsUpdates) {
  const auto& raw_node = *raw_;
  raw_->SetText(u"original");
  raw_->SetEndIndex(3);
  const auto owned_text = raw_->Text();
  EXPECT_EQ(raw_node.GetTruncatedTextView(), std::u16string_view(u"ori"));

  // Reacquire the view after each mutation; Text() still owns its snapshot.
  raw_->SetText(u"replacement");
  EXPECT_EQ(raw_node.GetTruncatedTextView(),
            std::u16string_view(u"replacement"));
  EXPECT_EQ(owned_text, u"ori");
  raw_->SetEndIndex(0);
  EXPECT_TRUE(raw_node.GetTruncatedTextView().empty());
  raw_->SetEndIndex(std::numeric_limits<size_t>::max());
  EXPECT_EQ(raw_node.GetTruncatedTextView(),
            std::u16string_view(u"replacement"));
  raw_->SetText(u"");
  EXPECT_TRUE(raw_node.GetTruncatedTextView().empty());
}

#if defined(CLAY_ENABLE_TTTEXT)
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
TEST_F_UI(TextLayoutReuseTest, MeasureSkipsOnlyEligibleSecondLayout) {
  hooks_->SetEnabled(true);
  for (const auto* text : {u"Text", u"Hello world", u"office 123"}) {
    raw_->SetText(text);
    const auto layouts = hooks_->Layouts(&Render());
    const auto scans = hooks_->TextScans(raw_.get());
    const auto first = Measure(1000.f);
    ASSERT_GT(first.width, 0.f);
    ASSERT_LT(first.width, 999.f);
    // Count before calling Reject(): Measure itself must run the policy.
    EXPECT_EQ(hooks_->Layouts(&Render()), layouts + 1);
    EXPECT_EQ(hooks_->TextScans(raw_.get()), scans + 1);
    ASSERT_EQ(Reject(first.width), nullptr);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()), 1000.f);
    auto* cached = Render().GetCacheParagraph();
    ASSERT_NE(cached, nullptr);

    for (int i = 0; i < 3; ++i) {
      const auto repeated = Measure(1000.f);
      EXPECT_FLOAT_EQ(repeated.width, first.width);
      EXPECT_FLOAT_EQ(repeated.height, first.height);
      EXPECT_EQ(Render().GetCacheParagraph(), cached);
      EXPECT_EQ(hooks_->Layouts(&Render()), layouts + 1);
      EXPECT_EQ(hooks_->TextScans(raw_.get()), scans + 1);
    }

    auto rebuilt = Render().LayoutParagraph(first.width);
    ASSERT_NE(rebuilt, nullptr);
    ExpectSameGeometry(*cached, *rebuilt, raw_->GetTruncatedTextView().size());

    // Even a constraint equal to the measured width must invalidate the
    // original requested-width cache. Do not infer rebuilds from addresses.
    const auto before_constrained = hooks_->Layouts(&Render());
    const auto constrained = Measure(first.width, MeasureMode::kDefinite);
    EXPECT_EQ(hooks_->Layouts(&Render()), before_constrained + 1);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                    first.width);
    EXPECT_FLOAT_EQ(constrained.width, first.width);
    EXPECT_FLOAT_EQ(constrained.height, first.height);
  }
}

TEST_F_UI(TextLayoutReuseTest, MeasureGateOverrideSupportsEnableAndRollback) {
  hooks_->SetEnabled(false);
  const auto baseline = Measure(1000.f);
  ASSERT_GT(baseline.width, 0.f);
  ASSERT_LT(baseline.width, 999.f);
  EXPECT_EQ(hooks_->Layouts(&Render()), 2u);
  // A disabled gate must avoid even the eligibility scan.
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 0u);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                  baseline.width);
  auto reference = Render().LayoutParagraph(baseline.width);
  ASSERT_NE(reference, nullptr);

  // Exercise both transitions on the same cached node, without changing the
  // real setting. Repeated enabled and disabled measurements differ in work,
  // but must preserve the same geometry.
  const struct {
    bool enabled;
    size_t layouts;
  } steps[] = {{true, 1}, {true, 0}, {false, 1}, {false, 2}, {true, 1}};
  for (const auto& step : steps) {
    SCOPED_TRACE(::testing::Message()
                 << "enabled=" << step.enabled << "layouts=" << step.layouts);
    hooks_->SetEnabled(step.enabled);
    const auto layouts = hooks_->Layouts(&Render());
    const auto result = Measure(1000.f);
    EXPECT_EQ(hooks_->Layouts(&Render()) - layouts, step.layouts);
    EXPECT_EQ(hooks_->TextScans(raw_.get()), 1u);
    EXPECT_FLOAT_EQ(result.width, baseline.width);
    EXPECT_FLOAT_EQ(result.height, baseline.height);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                    step.enabled ? 1000.f : result.width);
    auto* paragraph = Render().GetCacheParagraph();
    ASSERT_NE(paragraph, nullptr);
    ExpectSameGeometry(*reference, *paragraph,
                       raw_->GetTruncatedTextView().size());
  }
}

TEST_F_UI(TextLayoutReuseTest, ScopedTestGateRestoresThePreviousOverride) {
  hooks_->SetEnabled(true);
  {
    ScopedTextLayoutReuseTestHooks disabled;
    EXPECT_EQ(ScopedTextLayoutReuseTestHooks::GetEnabledOverride(), false);
    Measure(1000.f);
    EXPECT_EQ(disabled.Layouts(&Render()), 2u);
    EXPECT_EQ(hooks_->Layouts(&Render()), 0u);
  }
  EXPECT_EQ(ScopedTextLayoutReuseTestHooks::GetEnabledOverride(), true);
  Measure(1000.f);
  EXPECT_EQ(hooks_->Layouts(&Render()), 1u);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()), 1000.f);
}

TEST_F_UI(TextLayoutReuseTest, MeasureKeepsAnExistingSecondLayoutRequest) {
  hooks_->SetEnabled(true);
  node_->SetNeedSecondLayout(true);
  const auto result = Measure(1000.f);
  ASSERT_EQ(Reject(result.width), nullptr);
  EXPECT_EQ(hooks_->Layouts(&Render()), 2u);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                  result.width);
  // The old request is consumed, not retained for every later measurement.
  Measure(1000.f);
  EXPECT_EQ(hooks_->Layouts(&Render()), 3u);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()), 1000.f);
}

TEST_F_UI(TextLayoutReuseTest, EnabledMeasureStillFallsBackForRejectedText) {
  hooks_->SetEnabled(true);
  for (const auto* text : {u"Text ", u"Text\u3000", u"Cafe\u0301"}) {
    raw_->SetText(text);
    const auto layouts = hooks_->Layouts(&Render());
    const auto result = Measure(1000.f);
    ASSERT_GT(result.width, 0.f);
    ASSERT_LT(result.width, 999.f);
    EXPECT_NE(Reject(result.width), nullptr);
    EXPECT_EQ(hooks_->Layouts(&Render()) - layouts, 2u);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                    result.width);
  }
}

TEST_F_UI(TextLayoutReuseTest, CacheHitsDoNotRescanSupportedOrUnsupportedText) {
  for (const std::u16string text :
       {std::u16string(u"plain text"), std::u16string(u"Cafe\u0301"),
        std::u16string(16384, u'x'), std::u16string(16384, u'x') + u'\t'}) {
    raw_->SetText(text);
    const auto scans = hooks_->TextScans(raw_.get());
    const bool supported = raw_->IsTextSupportedForLayoutReuse();
    EXPECT_EQ(hooks_->TextScans(raw_.get()), scans + 1);
    for (int i = 0; i < 3; ++i) {
      raw_->SetText(text);
      EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), supported);
      EXPECT_EQ(hooks_->TextScans(raw_.get()), scans + 1);
    }
  }
}

TEST_F_UI(TextLayoutReuseTest, PrefixCacheMissScansOnceAndUnchangedPrefixHits) {
  raw_->SetText(u"ok\t");
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 1u);
  raw_->SetEndIndex(2);
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 2u);
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 2u);
  raw_->SetEndIndex(3);
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 3u);
  raw_->SetEndIndex(std::numeric_limits<size_t>::max());
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 3u);
}

TEST_F_UI(TextLayoutReuseTest, ChangingOneRawChildDoesNotRescanOtherChildren) {
  hooks_->SetEnabled(true);
  raw_->SetText(u"first");
  auto* changed = AddRawText(u"second");
  auto* last = AddRawText(u"last");
  const auto check = [&](size_t expected_layouts, size_t first_scans,
                         size_t changed_scans) {
    const auto layouts = hooks_->Layouts(&Render());
    const auto result = Measure(1000.f);
    ASSERT_GT(result.width, 0.f);
    ASSERT_LT(result.width, 999.f);
    EXPECT_EQ(hooks_->Layouts(&Render()) - layouts, expected_layouts);
    EXPECT_EQ(hooks_->TextScans(raw_.get()), first_scans);
    EXPECT_EQ(hooks_->TextScans(changed), changed_scans);
    EXPECT_EQ(hooks_->TextScans(last), 1u);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                    expected_layouts == 1 ? 1000.f : result.width);
  };
  check(1, 1, 1);
  changed->SetText(u"edited");
  check(1, 1, 2);
  changed->SetText(u"e\u0301");
  check(2, 1, 3);
  // A cached rejection in an unchanged sibling must still block reuse.
  raw_->SetText(u"other");
  check(2, 2, 3);
  check(2, 2, 3);
  changed->SetText(u"safe");
  check(1, 2, 4);
}

TEST_F_UI(TextLayoutReuseTest, RtlStartAndEndAlignmentKeepTheSecondLayout) {
  hooks_->SetEnabled(true);
  node_->SetTextDirection(TextDirection::kRtl);
  for (auto align : {TextAlignment::kStart, TextAlignment::kEnd}) {
    node_->SetTextAlign(align);
    const auto first = BuildFirstLayout(1000.f);
    ASSERT_GT(first.measured_width_, 0);
    ASSERT_LT(first.measured_width_, 999);
    EXPECT_STREQ(Reject(first.measured_width_), align == TextAlignment::kStart
                                                    ? "non_left_alignment"
                                                    : "non_ltr_direction");
    const auto layouts = hooks_->Layouts(&Render());
    const auto result = Measure(1000.f);
    // The first paragraph was already cached above; only the second is new.
    EXPECT_EQ(hooks_->Layouts(&Render()) - layouts, 1u);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                    result.width);
  }
}

TEST_F_UI(TextLayoutReuseTest, SupportedScriptsAndRawSegmentsPreserveGeometry) {
  hooks_->SetEnabled(true);
  auto* second = AddRawText(u"");
  auto* last = AddRawText(u"");
  const std::u16string cases[][3] = {
      {u"\u4e2d\u6587", u"\uff0cABC", u"\uff01"},
      {u"Hello ", u"\u4e16\u754c", u" 123"},
      {u"Caf\u00e9", u"\u3000\u4e2d\u6587", u"\uff11\uff12\uff13"},
      {u"\u201c\u65b0\u54c1\u201d", u"\u2014\u4e0a\u5e02", u"\u2026"},
      {u"of", u"", u"fice"},
  };
  for (auto word_break :
       {WordBreak::kNormal, WordBreak::kBreakAll, WordBreak::kKeepAll}) {
    node_->SetWordBreak(word_break);
    for (const auto& segments : cases) {
      SCOPED_TRACE(::testing::Message() << "word_break=" << word_break
                                        << "first_size=" << segments[0].size());
      raw_->SetText(segments[0]);
      second->SetText(segments[1]);
      last->SetText(segments[2]);
      const auto layouts = hooks_->Layouts(&Render());
      const auto result = Measure(1000.f);
      ASSERT_GT(result.width, 0.f);
      ASSERT_LT(result.width, 999.f);
      ASSERT_EQ(Reject(result.width), nullptr);
      EXPECT_EQ(hooks_->Layouts(&Render()) - layouts, 1u);
      auto* original = Render().GetCacheParagraph();
      ASSERT_NE(original, nullptr);
      auto rebuilt = Render().LayoutParagraph(result.width);
      ASSERT_NE(rebuilt, nullptr);
      ExpectSameGeometry(
          *original, *rebuilt,
          segments[0].size() + segments[1].size() + segments[2].size());
    }
  }
}
#endif  // defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)

TEST_F_UI(TextLayoutReuseTest, TextAllowlistRangeBoundaries) {
  const struct {
    char16_t first;
    char16_t last;
  } ranges[] = {{0x0020, 0x007E}, {0x00C0, 0x024F}, {0x2010, 0x2027},
                {0x3000, 0x303F}, {0x3400, 0x4DBF}, {0x4E00, 0x9FFF},
                {0xFF01, 0xFF60}, {0xFFE0, 0xFFE6}};
  const auto check = [&](char16_t code, bool expected) {
    SCOPED_TRACE(static_cast<unsigned>(code));
    raw_->SetText(std::u16string(1, code));
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), expected);
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), expected);
  };
  for (const auto& range : ranges) {
    check(range.first, true);
    check(range.last, true);
    check(static_cast<char16_t>(range.first - 1), false);
    check(static_cast<char16_t>(range.last + 1), false);
  }
}

TEST_F_UI(TextLayoutReuseTest, TextAllowlistRepresentativeInputs) {
  // These test only the character policy, independent of fonts and geometry.
  for (const auto* text :
       {u"ASCII 123 !", u"\u4f60\u597d\uff0c\u4e16\u754c\uff01",
        u"\u201c\u65b0\u54c1\u201d\u2014\u4e0a\u5e02\u2026",
        u"\u4ef7\u683c\uff1a\uffe5\uff11\uff12\uff13", u"Caf\u00e9 na\u00efve",
        u"\u3400\u4e00", u"a\u3000b"}) {
    raw_->SetText(text);
    EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  }
  // Unlisted text stays on the normal path. No normalization is performed:
  // a precomposed accented letter passes, but a combining sequence does not.
  for (const auto* text :
       {u"e\u0301", u"\u20ac", u"\u3042", u"\u0430", u"\ufe0f", u"\ufeff",
        u"\U0001f600", u"\U00020000", u"a\nb", u"a\u200db", u"a\u202eb",
        u"a\u2066b", u"\u007f"}) {
    raw_->SetText(text);
    EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  }
}

TEST_F_UI(TextLayoutReuseTest, CachedTextCheckTracksContentChanges) {
  const struct {
    const char16_t* text;
    bool supported;
  } cases[] = {{u"abc", true},   {u"a\tb", false}, {u"abc", true},
               {u"a\nb", false}, {u"xyz", true},   {u"a\u0301b", false},
               {u"abc", true}};
  for (const auto& test : cases) {
    raw_->SetText(test.text);
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), test.supported);
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), test.supported);
    // Reassigning identical text must preserve the result as well.
    raw_->SetText(test.text);
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), test.supported);
  }
  raw_->SetText(u"");
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  raw_->SetText(std::u16string(1, u'\0'));
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  raw_->SetText(u"");
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
}

TEST_F_UI(TextLayoutReuseTest, CachedTextCheckTracksEffectivePrefix) {
  raw_->SetText(u"ok\tblocked");
  for (size_t end : {size_t{2}, size_t{3}, size_t{0},
                     std::numeric_limits<size_t>::max(), size_t{2}}) {
    raw_->SetEndIndex(end);
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), end <= 2);
    EXPECT_EQ(raw_->IsTextSupportedForLayoutReuse(), end <= 2);
  }
  raw_->SetText(u"ok\tblocked");
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  node_->ResetEndIndex();
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());

  // Truncating inside a surrogate pair keeps the original code-unit policy.
  raw_->SetText(u"\U0001f600");
  raw_->SetEndIndex(0);
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  raw_->SetEndIndex(1);
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  raw_->SetEndIndex(2);
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
}

TEST_F_UI(TextLayoutReuseTest, LongTextCacheTracksTailChanges) {
  std::u16string text(16384, u'x');
  raw_->SetText(text);
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());

  // A same-length edit at the very end must invalidate a cached success.
  text.back() = u'\t';
  raw_->SetText(text);
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());
  raw_->SetEndIndex(text.size() - 1);
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
  raw_->SetEndIndex(text.size());
  EXPECT_FALSE(raw_->IsTextSupportedForLayoutReuse());

  text.back() = u'x';
  raw_->SetText(text);
  EXPECT_TRUE(raw_->IsTextSupportedForLayoutReuse());
}

TEST_F_UI(TextLayoutReuseTest, MissingInputsFailClosed) {
  TextRender no_node(nullptr);
  EXPECT_STREQ(TextRenderReuseTestPeer::Reject(no_node, 100),
               "no_measure_node_or_style");
  EXPECT_STREQ(Reject(), "no_paragraph");
  node_->text_style_.reset();
  EXPECT_STREQ(Reject(), "no_measure_node_or_style");
}

TEST_F_UI(TextLayoutReuseTest, CallerFeatureRestrictionsAreIndependent) {
  Measure(1000.f, MeasureMode::kDefinite);
  ASSERT_EQ(Reject(), nullptr);

  // Mutate one policy input at a time on an already valid paragraph. These
  // checks intentionally isolate caller restrictions from engine geometry.
  node_->enable_auto_font_size_ = true;
  EXPECT_STREQ(Reject(), "auto_font_size");
  node_->enable_auto_font_size_ = false;
  node_->text_indent_ = 1.f;
  EXPECT_STREQ(Reject(), "text_indent");
  node_->text_indent_ = 0.f;
  node_->text_indent_use_percent_ = true;
  EXPECT_STREQ(Reject(), "percent_text_indent");
  node_->text_indent_use_percent_ = false;
  node_->max_length_ = 0;
  EXPECT_STREQ(Reject(), "max_length");
  node_->max_length_.reset();
  node_->SetRichType("bracket");
  EXPECT_STREQ(Reject(), "bracket_rich_text");
  node_->SetRichType("");

  auto& style = *node_->text_style_;
  style.white_space = WhiteSpace::kNoWrap;
  EXPECT_STREQ(Reject(), "nowrap");
  style.white_space.reset();
  // An explicitly present zero is still excluded for indent and max_lines.
  style.text_indent = 0.f;
  EXPECT_STREQ(Reject(), "text_indent_style");
  style.text_indent.reset();
  style.max_lines = 0;
  EXPECT_STREQ(Reject(), "max_lines");
  style.max_lines.reset();
  for (float spacing : {-1.f, 1.f, std::numeric_limits<float>::quiet_NaN()}) {
    style.letter_spacing = spacing;
    EXPECT_STREQ(Reject(), "nonzero_letter_spacing");
  }
  style.letter_spacing = 0.f;
  EXPECT_EQ(Reject(), nullptr);
  style.letter_spacing.reset();

  auto inline_text =
      std::make_unique<InlineTextShadowNode>(owner_.get(), "inline-text", 3);
  node_->AddChild(inline_text.get());
  EXPECT_STREQ(Reject(), "complex_children");
  node_->RemoveChild(inline_text.get());
  EXPECT_EQ(Reject(), nullptr);
}

TEST_F_UI(TextLayoutReuseTest, RawTextPolicyRemainsConservative) {
  Measure(1000.f, MeasureMode::kDefinite);
  ASSERT_EQ(Reject(), nullptr);
  // Keep valid cached geometry to test the raw-input policy independently
  // of font coverage, whitespace collapsing and paragraph reconstruction.
  for (const auto* text :
       {u"a\tb", u"a\nb", u"a\u0085b", u"a\u2028b", u"a\u2029b", u"a\u200eb",
        u"a\u202ab", u"a\u2066b", u"\u05d0", u"\u0627", u"\ufb1d",
        u"\U0001f600", u"e\u0301", u"\u3042", u"\u20ac"}) {
    raw_->SetText(text);
    EXPECT_STREQ(Reject(), "unsupported_text");
  }
  raw_->SetText("");
  EXPECT_STREQ(Reject(), "empty_text");
  raw_->SetText("ordinary text");
  EXPECT_EQ(Reject(), nullptr);
}

TEST_F_UI(TextLayoutReuseTest, GeometryRejectionReachesCompletePolicy) {
  Measure(1000.f, MeasureMode::kDefinite);
  EXPECT_STREQ(Reject(std::numeric_limits<double>::quiet_NaN()),
               "invalid_target_width");
  EXPECT_STREQ(Reject(0), "invalid_target_width");
  EXPECT_STREQ(Reject(1), "target_too_narrow");
  node_->SetTextAlign(TextAlignment::kCenter);
  Measure(1000.f, MeasureMode::kDefinite);
  EXPECT_STREQ(Reject(), "non_left_alignment");
}

TEST_F_UI(TextLayoutReuseTest, RawTextSegmentsPreserveEmptyAndTruncatedText) {
  Measure(1000.f, MeasureMode::kDefinite);
  ASSERT_EQ(Reject(), nullptr);
  // Keep valid cached geometry to exercise only the raw-text policy.
  auto second =
      std::make_unique<RawTextShadowNode>(owner_.get(), "raw-text", 3);
  node_->AddChild(second.get());
  raw_->SetText("");
  second->SetText("");
  EXPECT_STREQ(Reject(), "empty_text");
  second->SetText("second");
  EXPECT_EQ(Reject(), nullptr);
  raw_->SetText("first");
  second->SetText("");
  EXPECT_EQ(Reject(), nullptr);

  second->SetText(u"ok\tblocked");
  second->SetEndIndex(2);
  EXPECT_EQ(Reject(), nullptr);
  second->SetEndIndex(std::numeric_limits<size_t>::max());
  EXPECT_STREQ(Reject(), "unsupported_text");
  raw_->SetEndIndex(0);
  second->SetEndIndex(0);
  EXPECT_STREQ(Reject(), "empty_text");

  // Surrogates remain excluded even when a pair spans two RawText children.
  raw_->SetText(std::u16string(1, static_cast<char16_t>(0xD83D)));
  second->SetText(std::u16string(1, static_cast<char16_t>(0xDE00)));
  EXPECT_STREQ(Reject(), "unsupported_text");
  raw_->SetText(std::u16string(1, u'\0'));
  second->SetText("");
  EXPECT_STREQ(Reject(), "unsupported_text");

  node_->RemoveChild(second.get());
  node_->RemoveChild(raw_.get());
  EXPECT_STREQ(Reject(), "empty_text");
  node_->AddChild(raw_.get());
}

TEST_F_UI(TextLayoutReuseTest, MultipleRejectionConditionsRemainBlocked) {
  Measure(1000.f, MeasureMode::kDefinite);
  ASSERT_EQ(Reject(), nullptr);
  raw_->SetText(u"a\tb");
  auto inline_text =
      std::make_unique<InlineTextShadowNode>(owner_.get(), "inline-text", 3);
  node_->AddChild(inline_text.get());
  // Multiple blockers must reject reuse without prescribing which wins.
  EXPECT_NE(Reject(), nullptr);
  node_->RemoveChild(inline_text.get());
  EXPECT_NE(Reject(0), nullptr);
  EXPECT_STREQ(Reject(), "unsupported_text");
}

TEST_F_UI(TextLayoutReuseTest, PercentIndentIsRejectedForAllValues) {
  Measure(1000.f, MeasureMode::kDefinite);
  ASSERT_EQ(Reject(), nullptr);
  for (float indent :
       {0.f, -0.f, 1.f, -1.f, std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()}) {
    node_->text_indent_ = indent;
    node_->text_indent_use_percent_ = true;
    EXPECT_NE(Reject(), nullptr);
    node_->text_indent_use_percent_ = false;
    if (indent != 0.f) {
      EXPECT_STREQ(Reject(), "text_indent");
    } else {
      EXPECT_EQ(Reject(), nullptr);
    }
  }
}

#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
TEST_F_UI(TextLayoutReuseTest, ReusedLayoutTracksContentAndStyleChanges) {
  hooks_->SetEnabled(true);
  const auto first = Measure(1000.f);
  ASSERT_EQ(hooks_->Layouts(&Render()), 1u);
  ASSERT_EQ(Reject(first.width), nullptr);

  // Keep the constraint unchanged throughout, including eligibility changes.
  std::u16string long_text;
  for (int i = 0; i < 32; ++i) {
    long_text += u"Hello world ";
  }
  long_text += u"end";
  raw_->SetText(long_text);
  const auto multiline = Measure(1000.f);
  EXPECT_GT(hooks_->Layouts(&Render()), 1u);
  EXPECT_STREQ(Reject(multiline.width), "not_single_line");
  EXPECT_GT(multiline.height, first.height);

  raw_->SetText(u"Hi");
  auto layouts = hooks_->Layouts(&Render());
  const auto shorter = Measure(1000.f);
  EXPECT_EQ(hooks_->Layouts(&Render()), layouts + 1);
  EXPECT_LT(shorter.height, multiline.height);
  EXPECT_LT(shorter.width, first.width);

  node_->SetFontSize(40.f);
  layouts = hooks_->Layouts(&Render());
  const auto larger = Measure(1000.f);
  EXPECT_EQ(hooks_->Layouts(&Render()), layouts + 1);
  EXPECT_GT(larger.width, shorter.width);
  EXPECT_GT(larger.height, shorter.height);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()), 1000.f);

  // A width-dependent style must disable reuse even with a cached text pass.
  const auto scans = hooks_->TextScans(raw_.get());
  for (float spacing : {1.f, -1.f, 0.f}) {
    node_->SetLetterSpacing(spacing);
    layouts = hooks_->Layouts(&Render());
    const auto result = Measure(1000.f);
    ASSERT_GT(result.width, 0.f);
    ASSERT_LT(result.width, 999.f);
    EXPECT_EQ(hooks_->Layouts(&Render()) - layouts, spacing == 0 ? 1u : 2u);
    EXPECT_EQ(hooks_->TextScans(raw_.get()), scans);
    EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                    spacing == 0 ? 1000.f : result.width);
    if (spacing == 0) {
      EXPECT_FLOAT_EQ(result.width, larger.width);
      EXPECT_FLOAT_EQ(result.height, larger.height);
    } else {
      EXPECT_STREQ(Reject(result.width), "nonzero_letter_spacing");
    }
  }
}
#endif  // defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
#endif  // defined(CLAY_ENABLE_TTTEXT)

TEST_F_UI(TextLayoutReuseTest, MeasurementsHonorLaterInvalidation) {
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  hooks_->SetEnabled(true);
#endif
  const auto wide = Measure(1000.f);
  ASSERT_GT(wide.width, 0.f);
  ASSERT_LT(wide.width, 999.f);
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  ASSERT_EQ(hooks_->Layouts(&Render()), 1u);
#endif
  const float narrower = std::floor(wide.width / 2.f);
  const auto narrow = Measure(narrower, MeasureMode::kDefinite);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()), narrower);
  EXPECT_GT(narrow.height, wide.height);

  // Equal constraints do not suppress content/style invalidation.
  raw_->SetText("Hi");
  const auto shorter = Measure(narrower, MeasureMode::kDefinite);
  EXPECT_LT(shorter.height, narrow.height);
  node_->SetFontSize(40.f);
  const auto larger = Measure(narrower, MeasureMode::kDefinite);
  EXPECT_GT(larger.height, shorter.height);
}

TEST_F_UI(TextLayoutReuseTest, OtherMeasureModesKeepTheirLayoutBehavior) {
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  hooks_->SetEnabled(true);
#endif
  const auto definite = Measure(1000.f, MeasureMode::kDefinite);
  EXPECT_FLOAT_EQ(definite.width, 1000.f);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()), 1000.f);
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  EXPECT_EQ(hooks_->Layouts(&Render()), 1u);
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 0u);
#endif

  const auto indefinite = Measure(1000.f, MeasureMode::kIndefinite);
  EXPECT_GT(indefinite.width, 0.f);
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  EXPECT_EQ(hooks_->Layouts(&Render()), 2u);
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 0u);
#endif

  node_->SetTextAlign(TextAlignment::kCenter);
  const auto result = node_->Measure({std::nullopt, MeasureMode::kIndefinite,
                                      std::nullopt, MeasureMode::kIndefinite});
  ASSERT_GT(result.width, 0.f);
  EXPECT_FLOAT_EQ(TextRenderReuseTestPeer::RequestedWidth(Render()),
                  result.width);
#if defined(CLAY_TEXT_LAYOUT_REUSE_TESTS)
  EXPECT_EQ(hooks_->Layouts(&Render()), 4u);
  EXPECT_EQ(hooks_->TextScans(raw_.get()), 0u);
#endif
}

}  // namespace clay

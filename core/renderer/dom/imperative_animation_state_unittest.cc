// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/imperative_animation_state.h"

#include <initializer_list>
#include <memory>
#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

using AnimationSource = ImperativeAnimationState::Source;

StyleMap MakeTimingStyles(const char* animation_name,
                          starlight::AnimationFillModeType fill_mode =
                              starlight::AnimationFillModeType::kNone,
                          starlight::AnimationPlayStateType play_state =
                              starlight::AnimationPlayStateType::kRunning) {
  StyleMap styles;
  styles.insert_or_assign(
      kPropertyIDAnimationName,
      CSSValue(animation_name, CSSValuePattern::STRING, CSSValueType::DEFAULT));
  styles.insert_or_assign(kPropertyIDAnimationDuration,
                          CSSValue(1000, CSSValuePattern::NUMBER));
  styles.insert_or_assign(kPropertyIDAnimationFillMode, CSSValue(fill_mode));
  styles.insert_or_assign(kPropertyIDAnimationPlayState, CSSValue(play_state));
  return styles;
}

fml::RefPtr<CSSKeyframesToken> MakeKeyframes(
    std::initializer_list<CSSPropertyID> properties) {
  CSSParserConfigs parser_configs;
  auto token = fml::AdoptRef(new CSSKeyframesToken(parser_configs));

  CSSKeyframesContent content;
  auto frame_styles = std::make_shared<StyleMap>();
  for (const auto property : properties) {
    frame_styles->insert_or_assign(property,
                                   CSSValue(1, CSSValuePattern::NUMBER));
  }
  content.insert_or_assign(0.f, std::move(frame_styles));
  token->SetKeyframesContent(std::move(content));
  return token;
}

void ExpectCleanupProperties(const CSSIDBitset& actual,
                             std::initializer_list<CSSPropertyID> expected) {
  CSSIDBitset expected_bitset;
  for (const auto property : expected) {
    expected_bitset.Set(property);
  }
  EXPECT_TRUE(actual == expected_bitset);
}

}  // namespace

TEST(ImperativeAnimationStateTest,
     ReplacingAnimateStartCleansOnlyPropertiesNoLongerAnimated) {
  ImperativeAnimationState state;
  auto first_keyframes = MakeKeyframes({kPropertyIDLeft, kPropertyIDOpacity});
  auto second_keyframes = MakeKeyframes({kPropertyIDOpacity, kPropertyIDWidth});

  auto first_mutation = state.RecordStart(
      AnimationSource::kAnimate, "js-generated-one", "generated-one", true,
      MakeTimingStyles("generated-one"), first_keyframes.get());
  EXPECT_FALSE(first_mutation.cleanup_properties.HasAny());
  EXPECT_TRUE(first_mutation.keyframes_to_remove.empty());
  EXPECT_TRUE(state.HasAnimationName("generated-one"));

  auto second_mutation = state.RecordStart(
      AnimationSource::kAnimate, "js-generated-two", "generated-two", true,
      MakeTimingStyles("generated-two"), second_keyframes.get());

  ExpectCleanupProperties(second_mutation.cleanup_properties,
                          {kPropertyIDLeft});
  ASSERT_EQ(second_mutation.keyframes_to_remove.size(), 1u);
  EXPECT_EQ(second_mutation.keyframes_to_remove[0],
            base::String("generated-one"));
  EXPECT_FALSE(state.HasAnimationName("generated-one"));
  EXPECT_TRUE(state.HasAnimationName("generated-two"));

  ExpectCleanupProperties(state.TakePendingCleanupProperties(),
                          {kPropertyIDLeft});
  EXPECT_FALSE(state.TakePendingCleanupProperties().HasAny());
}

TEST(ImperativeAnimationStateTest,
     AnimateV2StartKeepsIndependentAnimationRecords) {
  ImperativeAnimationState state;
  auto first_keyframes = MakeKeyframes({kPropertyIDLeft});
  auto second_keyframes = MakeKeyframes({kPropertyIDWidth});

  state.RecordStart(AnimationSource::kAnimateV2, "move-left", "move-left",
                    false, MakeTimingStyles("move-left"),
                    first_keyframes.get());
  auto mutation = state.RecordStart(
      AnimationSource::kAnimateV2, "grow-width", "grow-width", false,
      MakeTimingStyles("grow-width"), second_keyframes.get());

  EXPECT_FALSE(mutation.cleanup_properties.HasAny());
  EXPECT_TRUE(mutation.keyframes_to_remove.empty());
  EXPECT_TRUE(state.HasAnimationName("move-left"));
  EXPECT_TRUE(state.HasAnimationName("grow-width"));

  starlight::ComputedCSSStyle computed_style(1.f, 1.f);
  state.ReplayToStyle(computed_style);
  ASSERT_TRUE(computed_style.HasAnimation());
  EXPECT_EQ(computed_style.animation_data().size(), 2u);
}

TEST(ImperativeAnimationStateTest,
     PlayStateUpdateReplaysWithOriginalAnimationName) {
  ImperativeAnimationState state;
  auto keyframes = MakeKeyframes({kPropertyIDOpacity});

  state.RecordStart(AnimationSource::kAnimateV2, "fade", "fade", false,
                    MakeTimingStyles("fade"), keyframes.get());

  StyleMap paused_styles;
  paused_styles.insert_or_assign(
      kPropertyIDAnimationPlayState,
      CSSValue(starlight::AnimationPlayStateType::kPaused));
  state.UpdatePlayState(AnimationSource::kAnimateV2, "fade", paused_styles,
                        true);

  starlight::ComputedCSSStyle computed_style(1.f, 1.f);
  state.ReplayToStyle(computed_style);
  ASSERT_TRUE(computed_style.HasAnimation());
  ASSERT_EQ(computed_style.animation_data().size(), 1u);
  EXPECT_EQ(computed_style.animation_data()[0].name, base::String("fade"));
  EXPECT_EQ(computed_style.animation_data()[0].play_state,
            starlight::AnimationPlayStateType::kPaused);
}

TEST(ImperativeAnimationStateTest,
     FinishRemovesNonForwardFillingAnimationAndKeepsForwardFillingAnimation) {
  ImperativeAnimationState state;
  auto transient_keyframes = MakeKeyframes({kPropertyIDTop});
  auto forwards_keyframes = MakeKeyframes({kPropertyIDWidth});

  state.RecordStart(AnimationSource::kAnimateV2, "transient", "transient", true,
                    MakeTimingStyles("transient"), transient_keyframes.get());
  state.RecordStart(
      AnimationSource::kAnimateV2, "persistent", "persistent", true,
      MakeTimingStyles("persistent",
                       starlight::AnimationFillModeType::kForwards),
      forwards_keyframes.get());

  auto transient_mutation =
      state.Finish(AnimationSource::kAnimateV2, "transient");
  ExpectCleanupProperties(transient_mutation.cleanup_properties,
                          {kPropertyIDTop});
  ASSERT_EQ(transient_mutation.keyframes_to_remove.size(), 1u);
  EXPECT_EQ(transient_mutation.keyframes_to_remove[0],
            base::String("transient"));
  EXPECT_FALSE(state.HasAnimationName("transient"));

  auto persistent_mutation =
      state.Finish(AnimationSource::kAnimateV2, "persistent");
  EXPECT_FALSE(persistent_mutation.cleanup_properties.HasAny());
  EXPECT_TRUE(persistent_mutation.keyframes_to_remove.empty());
  EXPECT_TRUE(state.HasAnimationName("persistent"));

  starlight::ComputedCSSStyle computed_style(1.f, 1.f);
  state.ReplayToStyle(computed_style);
  ASSERT_TRUE(computed_style.HasAnimation());
  ASSERT_EQ(computed_style.animation_data().size(), 1u);
  EXPECT_EQ(computed_style.animation_data()[0].name,
            base::String("persistent"));
  EXPECT_EQ(computed_style.animation_data()[0].fill_mode,
            starlight::AnimationFillModeType::kForwards);
}

TEST(ImperativeAnimationStateTest,
     ClearReturnsAffectedPropertiesAndResetsPendingCleanup) {
  ImperativeAnimationState state;
  auto old_keyframes = MakeKeyframes({kPropertyIDTop});
  auto generated_keyframes =
      MakeKeyframes({kPropertyIDOpacity, kPropertyIDWidth});
  auto named_keyframes = MakeKeyframes({kPropertyIDLeft});

  state.RecordStart(AnimationSource::kAnimate, "old-generated", "old-generated",
                    true, MakeTimingStyles("old-generated"),
                    old_keyframes.get());
  state.RecordStart(
      AnimationSource::kAnimate, "generated", "generated", true,
      MakeTimingStyles("generated",
                       starlight::AnimationFillModeType::kForwards),
      generated_keyframes.get());
  state.RecordStart(AnimationSource::kAnimateV2, "named", "named", false,
                    MakeTimingStyles("named"), named_keyframes.get());

  auto mutation = state.Clear();

  ExpectCleanupProperties(
      mutation.cleanup_properties,
      {kPropertyIDOpacity, kPropertyIDWidth, kPropertyIDLeft});
  ASSERT_EQ(mutation.keyframes_to_remove.size(), 1u);
  EXPECT_EQ(mutation.keyframes_to_remove[0], base::String("generated"));
  EXPECT_FALSE(state.HasRecords());
  EXPECT_FALSE(state.TakePendingCleanupProperties().HasAny());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/animation/css_keyframe_manager.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "core/animation/animation.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/testing/mock_css_keyframe_manager.h"
#include "core/animation/transform_animation_curve.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/inspector/observer/inspector_animation_observer.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "gfx/animation/capabilities/ios_animation_capabilities_generated.h"
#include "gfx/animation/timing_function.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class RoutingMockPaintingContext : public MockPaintingContext {
 public:
  void ApplyPlatformAnimationCommands(
      int id,
      std::shared_ptr<gfx::PlatformAnimationCommandBatch> commands) override {
    if (on_animation_commands) {
      on_animation_commands(id, commands);
    }
  }

  std::function<void(
      int, const std::shared_ptr<gfx::PlatformAnimationCommandBatch>&)>
      on_animation_commands;

  void SetPlatformAnimationCapabilities(
      gfx::AnimationBackendCapabilities capabilities) {
    capabilities_ = std::move(capabilities);
  }

  const gfx::AnimationBackendCapabilities& GetPlatformAnimationCapabilities()
      override {
    if (capabilities_.has_value()) {
      return *capabilities_;
    }
    return gfx::GetIOSAnimationBackendCapabilities();
  }

 private:
  std::optional<gfx::AnimationBackendCapabilities> capabilities_;
};

namespace {
class ScopedAnimationRoutingSetting {
 public:
  explicit ScopedAnimationRoutingSetting(const std::string& value) {
    auto& env = LynxEnv::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    auto it = env.external_env_map_.find(kKey);
    if (it != env.external_env_map_.end()) {
      previous_value_ = it->second;
    }
    env.external_env_map_[kKey] = value;
  }

  ~ScopedAnimationRoutingSetting() {
    auto& env = LynxEnv::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    if (previous_value_) {
      env.external_env_map_[kKey] = *previous_value_;
    } else {
      env.external_env_map_.erase(kKey);
    }
  }

  ScopedAnimationRoutingSetting(const ScopedAnimationRoutingSetting&) = delete;
  ScopedAnimationRoutingSetting& operator=(
      const ScopedAnimationRoutingSetting&) = delete;

 private:
  static constexpr auto kKey = LynxEnv::Key::ENABLE_ANIMATION_ROUTING;
  std::optional<std::string> previous_value_;
};
}  // namespace

TEST(AnimationRoutingSettingsTest, RoutingPolicyIsCapturedWhenPageIsCreated) {
  LynxEnvConfig env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                           kDefaultPhysicalPixelsPerLayoutUnit);
  ::testing::NiceMock<test::MockTasmDelegate> delegate;
  auto create_page = [&](bool has_backend) {
    auto painting_context = std::make_unique<RoutingMockPaintingContext>();
    if (!has_backend) {
      painting_context->SetPlatformAnimationCapabilities({});
    }
    auto page = std::make_unique<ElementManager>(std::move(painting_context),
                                                 &delegate, env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    page->SetConfig(config);
    return page;
  };

  // An empty setting uses the default; an explicit false also keeps routing
  // off.
  for (const auto* value : {"", "false", "true"}) {
    ScopedAnimationRoutingSetting setting(value);
    const bool enabled = std::string(value) == "true";
    for (bool has_backend : {false, true}) {
      auto page = create_page(has_backend);
      const bool routing = enabled && has_backend;
      EXPECT_EQ(page->SupportsPlatformAnimationRouting(), routing);
      for (bool new_animator : {false, true}) {
        auto element = page->CreateFiberElement("view");
        element->enable_new_animator_ = new_animator;
        EXPECT_EQ(element->supports_platform_animation_routing(), routing);
        EXPECT_EQ(element->use_cpp_animation_builder(),
                  routing || new_animator);
#if !OS_HARMONY
        for (bool explicit_value : {false, true}) {
          for (bool as_string : {false, true}) {
            SCOPED_TRACE(explicit_value);
            SCOPED_TRACE(as_string);
            element->SetAttribute(
                base::String("enable-new-animator"),
                as_string ? lepus::Value(explicit_value ? "true" : "false")
                          : lepus::Value(explicit_value));
            element->ConsumeAllAttributes();
            EXPECT_FALSE(element->supports_platform_animation_routing());
            EXPECT_EQ(element->use_cpp_animation_builder(), explicit_value);
          }
        }
        element->SetAttribute(base::String("enable-new-animator"),
                              lepus::Value());
        element->ConsumeAllAttributes();
        EXPECT_EQ(element->supports_platform_animation_routing(), routing);
        EXPECT_EQ(element->enable_new_animator(),
                  page->GetEnableNewAnimatorForFiber());
#endif
      }

      // A changed setting applies only to new pages, in either direction.
      ScopedAnimationRoutingSetting updated_setting(enabled ? "false" : "true");
      EXPECT_EQ(page->SupportsPlatformAnimationRouting(), routing);
      auto next_page = create_page(has_backend);
      EXPECT_EQ(next_page->SupportsPlatformAnimationRouting(),
                !enabled && has_backend);
    }
  }
}

class CSSKeyframeManagerTest : public ::testing::Test {
 public:
  CSSKeyframeManagerTest() {}
  ~CSSKeyframeManagerTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<lynx::tasm::Element> element_;
  RoutingMockPaintingContext* routing_painting_context_{nullptr};

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto painting_context = std::make_unique<RoutingMockPaintingContext>();
    routing_painting_context_ = painting_context.get();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::move(painting_context), tasm_mediator.get(), lynx_env_config);
    // Routing tests opt in explicitly; production pages default to routing off.
    manager->supports_platform_animation_routing_ = true;
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }

  std::shared_ptr<animation::Animation> InitTestAnimation() {
    auto test_animation =
        std::make_shared<animation::Animation>("test_animation");
    auto effect = animation::KeyframeEffect::Create();
    test_animation->SetKeyframeEffect(std::move(effect));
    element_ = manager->CreateFiberElement("view");
    test_animation->BindElement(element_.get());
    return test_animation;
  }

  void InitTestEffect(animation::KeyframeEffect& test_effect) {
    std::unique_ptr<animation::KeyframedOpacityAnimationCurve> test_curve(
        animation::KeyframedOpacityAnimationCurve::Create());

    auto test_frame1 = gfx::FloatKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetValue(1.0f);
    test_curve->AddKeyframe(std::move(test_frame1));
    test_curve->type_ = animation::AnimationCurve::CurveType::OPACITY;
    auto test_frame2 =
        gfx::FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame2->SetValue(0.0f);
    test_curve->AddKeyframe(std::move(test_frame2));
    std::unique_ptr<animation::KeyframeModel> new_model =
        animation::KeyframeModel::Create(std::move(test_curve));
    test_effect.AddKeyframeModel(std::move(new_model));
  }

  std::unique_ptr<animation::MockCSSKeyframeManager> InitTestKeyframeManager(
      tasm::Element* element) {
    auto test_manager =
        std::make_unique<animation::MockCSSKeyframeManager>(element);
    return test_manager;
  }

  starlight::AnimationData InitAnimationData(
      const base::String& name, long duration, long delay,
      starlight::TimingFunctionData timing_func, int iteration_count,
      starlight::AnimationFillModeType fill_mode,
      starlight::AnimationDirectionType direction,
      starlight::AnimationPlayStateType play_state) {
    starlight::AnimationData data;
    data.name = name;
    data.duration = duration;
    data.delay = delay;
    data.timing_func = timing_func;
    data.iteration_count = iteration_count;
    data.fill_mode = fill_mode;
    data.direction = direction;
    data.play_state = play_state;
    return data;
  }

  gfx::AnimationBackendCapabilities MakeIOSCapabilities() {
    return gfx::GetIOSAnimationBackendCapabilities();
  }

  fml::RefPtr<Element> InitElement() {
    auto test_element = manager->CreateFiberElement("view");
    // Enable the default C++ path without an explicit frontend override.
    test_element->enable_new_animator_ = true;
    return test_element;
  }

  void UpdateOpacityKeyframes(tasm::Element* element, const base::String& name,
                              double from, double to) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("opacity", lepus::Value(from));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateOpacityKeyframeTiming(tasm::Element* element,
                                   const base::String& name,
                                   const char* timing) {
    auto keyframes = lepus::Dictionary::Create();
    for (const auto& [offset, opacity] :
         std::vector<std::pair<const char*, double>>{
             {"0", 0.0}, {"50", 0.5}, {"100", 1.0}}) {
      auto frame = lepus::Dictionary::Create();
      frame->SetValue("opacity", lepus::Value(opacity));
      if (timing != nullptr) {
        frame->SetValue("animation-timing-function", lepus::Value(timing));
      }
      keyframes->SetValue(offset, lepus::Value(frame));
    }
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes),
        element->element_manager()->GetCSSParserConfigs());
  }

  void UpdateOutOfOrderOpacityKeyframes(tasm::Element* element,
                                        const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    for (const auto& [offset, opacity] :
         std::vector<std::pair<const char*, double>>{
             {"100", 1.0}, {"50", 0.5}, {"0", 0.0}}) {
      auto frame = lepus::Dictionary::Create();
      frame->SetValue("opacity", lepus::Value(opacity));
      keyframes->SetValue(offset, lepus::Value(frame));
    }

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateToOnlyOpacityKeyframes(tasm::Element* element,
                                    const base::String& name, double to) {
    auto keyframes = lepus::Dictionary::Create();
    keyframes->SetValue("0", lepus::Value(lepus::Dictionary::Create()));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateLeftKeyframes(tasm::Element* element, const base::String& name,
                           const char* from, const char* to) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("left", lepus::Value(from));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("left", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateToOnlyLeftKeyframes(tasm::Element* element,
                                 const base::String& name, const char* to) {
    auto keyframes = lepus::Dictionary::Create();
    keyframes->SetValue("0", lepus::Value(lepus::Dictionary::Create()));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("left", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateOpacityAndLeftKeyframes(tasm::Element* element,
                                     const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("opacity", lepus::Value(0.0));
    from_frame->SetValue("left", lepus::Value("0px"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(1.0));
    to_frame->SetValue("left", lepus::Value("100px"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateOpacityAndTransformKeyframes(
      tasm::Element* element, const base::String& name,
      const char* end_transform = "translateX(100px)") {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("opacity", lepus::Value(0.0));
    from_frame->SetValue("transform", lepus::Value("translateX(0px)"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(1.0));
    to_frame->SetValue("transform", lepus::Value(end_transform));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateRasterColorKeyframes(tasm::Element* element,
                                  const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("opacity", lepus::Value(0.0));
    from_frame->SetValue("background-color", lepus::Value("#ff000000"));
    from_frame->SetValue("color", lepus::Value("#ff000000"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(1.0));
    to_frame->SetValue("background-color", lepus::Value("#ffffffff"));
    to_frame->SetValue("color", lepus::Value("#ffffffff"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateWidthKeyframesWithDirectInheritedVariable(
      tasm::Element* element, const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("width", lepus::Value("var(--base)"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("width", lepus::Value("240px"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateWidthKeyframesWithLocalCustomProperty(tasm::Element* element,
                                                   const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("--x", lepus::Value("var(--base)"));
    from_frame->SetValue("width", lepus::Value("var(--x)"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("--x", lepus::Value("480px"));
    to_frame->SetValue("width", lepus::Value("var(--x)"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateCustomPropertyOnlyKeyframes(tasm::Element* element,
                                         const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("--x", lepus::Value("10px"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("--x", lepus::Value("20px"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  animation::LayoutKeyframe* FirstWidthKeyframe(
      animation::MockCSSKeyframeManager* manager, const base::String& name) {
    auto animation_iter = manager->animations_map().find(name);
    if (animation_iter == manager->animations_map().end()) {
      return nullptr;
    }
    auto* model =
        animation_iter->second->keyframe_effect()->GetKeyframeModelByCurveType(
            animation::AnimationCurve::CurveType::WIDTH);
    if (model == nullptr) {
      return nullptr;
    }
    auto* curve = static_cast<animation::KeyframedLayoutAnimationCurve*>(
        model->animation_curve());
    if (curve->keyframes_.empty()) {
      return nullptr;
    }
    return static_cast<animation::LayoutKeyframe*>(curve->keyframes_[0].get());
  }

  const tasm::CSSValue* FindSampledStyle(
      const animation::AnimationSampleForNewPipeline& sample,
      tasm::CSSPropertyID id) {
    auto iter = sample.property_overrides.find(id);
    return iter == sample.property_overrides.end() ? nullptr : &iter->second;
  }

  const tasm::CSSValue* FindSampledCustomProperty(
      const animation::AnimationSampleForNewPipeline& sample,
      const base::String& name) {
    auto iter = sample.custom_property_overrides.find(name);
    return iter == sample.custom_property_overrides.end() ? nullptr
                                                          : &iter->second;
  }

  fml::TimePoint TimePointFromMs(int64_t ms) {
    return fml::TimePoint::FromTicks(ms * 1000 * 1000);
  }
};

TEST_F(CSSKeyframeManagerTest, ConstructModel) {
  auto test_element = manager->CreateFiberElement("view");
  const auto underlying_opacity = CSSValue(0.75f, CSSValuePattern::NUMBER);
  ASSERT_TRUE(test_element->computed_css_style()->SetValue(
      kPropertyIDOpacity, CSSValue(0.2f, CSSValuePattern::NUMBER)));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto test_curve = animation::KeyframedOpacityAnimationCurve::Create();
  test_curve->SetUnderlyingValue(underlying_opacity);
  auto test_type = animation::AnimationCurve::CurveType::OPACITY;
  auto test_animation = InitTestAnimation();
  auto test_model = test_manager->ConstructModel(
      std::move(test_curve), test_type, test_animation.get());
  EXPECT_EQ(test_model->animation_curve()->Type(), test_type);
  EXPECT_EQ(test_model->animation_curve()->timing_function(), nullptr);
  EXPECT_EQ(test_model->animation_curve()->scaled_duration(),
            test_animation->get_animation_data().duration / 1000.0);
  EXPECT_EQ(test_model->animation_curve()->underlying_value_,
            underlying_opacity);
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineRefreshesUnderlyingValueAfterBaseResetAndUpdate) {
  auto test_element = InitElement();
  auto* base_style = test_element->computed_css_style();
  ASSERT_TRUE(base_style->SetValue(kPropertyIDOpacity,
                                   CSSValue(0.5f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  auto first_mid_time = TimePointFromMs(1500);
  auto first_mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(first_mid_time);
  const auto* first_mid_opacity =
      FindSampledStyle(first_mid_sample, kPropertyIDOpacity);
  ASSERT_NE(nullptr, first_mid_opacity);
  EXPECT_NEAR(0.25, first_mid_opacity->AsNumber(), 0.001);

  ASSERT_TRUE(base_style->ResetValue(kPropertyIDOpacity));
  EXPECT_EQ(base_style->GetResolvedValues().end(),
            base_style->GetResolvedValues().find(kPropertyIDOpacity));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto reset_mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(first_mid_time);
  const auto* reset_mid_opacity =
      FindSampledStyle(reset_mid_sample, kPropertyIDOpacity);
  ASSERT_NE(nullptr, reset_mid_opacity);
  EXPECT_NEAR(0.5, reset_mid_opacity->AsNumber(), 0.001);

  ASSERT_TRUE(base_style->SetValue(kPropertyIDOpacity,
                                   CSSValue(0.8f, CSSValuePattern::NUMBER)));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto updated_mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(first_mid_time);
  const auto* updated_mid_opacity =
      FindSampledStyle(updated_mid_sample, kPropertyIDOpacity);
  ASSERT_NE(nullptr, updated_mid_opacity);
  EXPECT_NEAR(0.4, updated_mid_opacity->AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       LegacyPipelineRefreshesUnderlyingValueAfterBaseResetAndUpdate) {
  auto test_element = manager->CreateFiberElement("view");
  ASSERT_TRUE(test_element->computed_css_style()->SetValue(
      kPropertyIDOpacity, CSSValue(0.5f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedOpacityAnimationCurve*>(
      model->animation_curve());
  test_element->css_keyframe_manager_ = std::move(test_manager);

  test_element->SetStyleInternal(kPropertyIDOpacity,
                                 CSSValue(0.8f, CSSValuePattern::NUMBER));
  auto mid_time = fml::TimeDelta::FromSecondsF(0.5);
  EXPECT_NEAR(0.4, curve->GetValue(mid_time).AsNumber(), 0.001);

  test_element->FlushAnimatedStyleInternal(
      kPropertyIDOpacity, CSSValue(0.4f, CSSValuePattern::NUMBER));
  EXPECT_NEAR(0.4, curve->GetValue(mid_time).AsNumber(), 0.001);

  test_element->ResetStyleInternal(kPropertyIDOpacity);
  EXPECT_NEAR(0.5, curve->GetValue(mid_time).AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       LegacyLayoutOnlyResetUsesDefaultUnderlyingValue) {
  auto test_element = manager->CreateFiberElement("view");
  ASSERT_FALSE(test_element->EnableLayoutInElementMode());
  UpdateToOnlyLeftKeyframes(test_element.get(), base::String("test"), "100px");

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::LEFT);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedLayoutAnimationCurve*>(
      model->animation_curve());
  test_element->css_keyframe_manager_ = std::move(test_manager);

  test_element->SetStyleInternal(kPropertyIDLeft,
                                 CSSValue(20.f, CSSValuePattern::PX));
  const auto& resolved_values =
      test_element->computed_css_style()->GetResolvedValues();
  EXPECT_EQ(resolved_values.end(), resolved_values.find(kPropertyIDLeft));
  auto mid_time = fml::TimeDelta::FromSecondsF(0.5);
  EXPECT_NEAR(60.f, curve->GetValue(mid_time).AsNumber(), 0.001);

  test_element->ResetStyleInternal(kPropertyIDLeft);
  EXPECT_EQ(resolved_values.end(), resolved_values.find(kPropertyIDLeft));
  EXPECT_NEAR(100.f, curve->GetValue(mid_time).AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       BuilderCreatesNewAnimatorModelsWithAnimationData) {
  auto capabilities = MakeIOSCapabilities();
  capabilities.backend = gfx::AnimationBackendType::kNone;
  routing_painting_context_->SetPlatformAnimationCapabilities(
      std::move(capabilities));
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  UpdateOpacityKeyframes(target.get(), base::String("test"), 0.0, 1.0);
  auto& keyframes =
      (*target->keyframes_map_)[base::String("test")]->GetKeyframesContent();
  tasm::CSSParserConfigs configs;
  for (auto& [offset, styles] : keyframes) {
    tasm::UnitHandler::Process(tasm::kPropertyIDLeft, lepus::Value("100px"),
                               *styles, configs);
    tasm::UnitHandler::Process(tasm::kPropertyIDColor, lepus::Value("blue"),
                               *styles, configs);
  }
  base::Vector<starlight::AnimationData> data{InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning)};
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  auto animation = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(animation, nullptr);
  ASSERT_EQ(animation->keyframe_effect()->keyframe_models().size(), 3u);
  for (auto type : {animation::AnimationCurve::CurveType::LEFT,
                    animation::AnimationCurve::CurveType::OPACITY,
                    animation::AnimationCurve::CurveType::TEXTCOLOR}) {
    auto* model =
        animation->keyframe_effect()->GetKeyframeModelByCurveType(type);
    ASSERT_NE(model, nullptr);
    EXPECT_TRUE(model->HasAnimationData());
    EXPECT_EQ(model->get_animation_data(), data[0]);
  }
}

TEST_F(CSSKeyframeManagerTest, BuilderRejectsNonNumericOpacityLikeLegacy) {
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  UpdateOpacityKeyframes(target.get(), base::String("test"), 0.0, 1.0);
  auto& keyframes =
      (*target->keyframes_map_)[base::String("test")]->GetKeyframesContent();
  (*keyframes.at(0.0))[tasm::kPropertyIDOpacity] =
      tasm::CSSValue(2.f, tasm::CSSValuePattern::REM);
  (*keyframes.at(1.0))[tasm::kPropertyIDOpacity] =
      tasm::CSSValue("calc(1rem + 10px)", tasm::CSSValuePattern::CALC,
                     tasm::CSSValueType::DEFAULT);
  base::Vector<starlight::AnimationData> data{InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning)};
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  EXPECT_TRUE(keyframe_manager->animations_map().empty());
  EXPECT_TRUE(keyframe_manager->platform_animations_.empty());
}

TEST_F(CSSKeyframeManagerTest, GetDefaultValue) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto default_value1 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kLeft);
  EXPECT_EQ(default_value1, tasm::CSSValue());

  auto default_value2 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kOpacity);
  EXPECT_EQ(default_value2, tasm::CSSValue(animation::kDefaultOpacity,
                                           tasm::CSSValuePattern::NUMBER));

  auto default_value3 = test_manager->GetDefaultValue(
      starlight::AnimationPropertyType::kBackgroundColor);
  EXPECT_EQ(default_value3, tasm::CSSValue(animation::kDefaultBackgroundColor,
                                           tasm::CSSValuePattern::NUMBER));

  auto default_value4 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kNone);
  EXPECT_EQ(default_value4, tasm::CSSValue());
}

TEST_F(CSSKeyframeManagerTest, HasTwoSameAnimation) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 3000, 100, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_EQ(animation::Animation::Origin::kCSSAnimation,
            test_manager->animations_map()[base::String("test")]->GetOrigin());
  EXPECT_TRUE(test_manager->animations_map()[base::String("test")]
                  ->get_animation_data()
                  .duration == 3000);
}

TEST_F(CSSKeyframeManagerTest, ClearEffect) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_TRUE(test_manager->animations_map()[base::String("test")]
                  ->get_animation_data()
                  .duration == 2000);
  animation_data.clear();
  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->animations_map().empty());
  EXPECT_TRUE(test_manager->GetClearEffectAnimationName() == "test");
}

TEST_F(CSSKeyframeManagerTest, DurationZero) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 0, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->has_flush_animated_style());
  EXPECT_TRUE(
      test_manager->animations_map()[base::String("test")]->GetState() ==
      animation::Animation::State::kStop);
}

TEST_F(CSSKeyframeManagerTest, DurationLessThanZero) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), -1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);
  ASSERT_EQ(1U, test_manager->animation_data_.size());
  EXPECT_EQ(0, test_manager->animation_data_[0].duration);
  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_EQ(0, test_manager->animations_map()[base::String("test")]
                   ->get_animation_data()
                   .duration);
}

TEST_F(CSSKeyframeManagerTest,
       ResolvesKeyframeCustomPropertyFromElementCustomProperty) {
  auto test_element = InitElement();
  test_element->computed_css_style()->SetCustomProperty(
      base::String("--base"), tasm::CSSValue::MakePlainString("120px"));
  test_element->computed_css_style()->FinalizeCustomProperties();

  auto keyframes = lepus::Dictionary::Create();
  auto from_frame = lepus::Dictionary::Create();
  from_frame->SetValue("--x", lepus::Value("var(--base)"));
  from_frame->SetValue("width", lepus::Value("var(--x)"));
  keyframes->SetValue("0", lepus::Value(from_frame));
  auto to_frame = lepus::Dictionary::Create();
  to_frame->SetValue("--x", lepus::Value("240px"));
  to_frame->SetValue("width", lepus::Value("var(--x)"));
  keyframes->SetValue("100", lepus::Value(to_frame));

  lynx::tasm::CSSParserConfigs configs;
  starlight::CSSStyleUtils::UpdateCSSKeyframes(
      *test_element->keyframes_map_, base::String("test"),
      lepus::Value(keyframes), configs);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::WIDTH);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedLayoutAnimationCurve*>(
      model->animation_curve());
  ASSERT_EQ(2U, curve->keyframes_.size());
  auto* from_keyframe =
      static_cast<animation::LayoutKeyframe*>(curve->keyframes_[0].get());
  EXPECT_EQ(tasm::CSSValuePattern::PX, from_keyframe->CSSValue().GetPattern());
  EXPECT_EQ(120, from_keyframe->CSSValue().AsNumber());
}

TEST_F(CSSKeyframeManagerTest,
       ResolvesDirectInheritedCustomPropertyInKeyframeValue) {
  auto test_element = InitElement();
  test_element->computed_css_style()->SetCustomProperty(
      base::String("--base"), tasm::CSSValue::MakePlainString("120px"));
  test_element->computed_css_style()->FinalizeCustomProperties();
  UpdateWidthKeyframesWithDirectInheritedVariable(test_element.get(),
                                                  base::String("test"));

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);

  auto* from_keyframe =
      FirstWidthKeyframe(test_manager.get(), base::String("test"));
  ASSERT_NE(nullptr, from_keyframe);
  EXPECT_EQ(tasm::CSSValuePattern::PX, from_keyframe->CSSValue().GetPattern());
  EXPECT_EQ(120, from_keyframe->CSSValue().AsNumber());
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineSyncUsesProvidedBaseCustomPropertiesForKeyframes) {
  auto test_element = InitElement();
  test_element->computed_css_style()->SetCustomProperty(
      base::String("--base"), tasm::CSSValue::MakePlainString("120px"));
  test_element->computed_css_style()->FinalizeCustomProperties();
  UpdateWidthKeyframesWithLocalCustomProperty(test_element.get(),
                                              base::String("test"));

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  tasm::CustomPropertiesMap new_base_custom_properties;
  new_base_custom_properties.insert_or_assign(
      base::String("--base"), tasm::CSSValue::MakePlainString("240px"));

  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, true, nullptr, nullptr, &new_base_custom_properties);

  auto* from_keyframe =
      FirstWidthKeyframe(test_manager.get(), base::String("test"));
  ASSERT_NE(nullptr, from_keyframe);
  EXPECT_EQ(tasm::CSSValuePattern::PX, from_keyframe->CSSValue().GetPattern());
  EXPECT_EQ(240, from_keyframe->CSSValue().AsNumber());
}

TEST_F(CSSKeyframeManagerTest,
       CustomPropertyOnlyAnimationDoesNotDuplicateStartEventAfterDummySample) {
  auto test_element = InitElement();
  UpdateCustomPropertyOnlyKeyframes(test_element.get(), base::String("test"));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  auto dummy_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  EXPECT_FALSE(dummy_sample.custom_property_overrides.empty());
  auto dummy_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, dummy_events.size());
  EXPECT_TRUE(dummy_events[0].send_start_event);

  auto real_time = fml::TimePoint::FromTicks(16 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(real_time);
  auto real_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  for (const auto& event : real_events) {
    EXPECT_FALSE(event.send_start_event);
  }
}

TEST_F(CSSKeyframeManagerTest,
       CustomPropertyOnlyAnimationSendsEndOnceAndKeepsRepeatedFillSample) {
  auto test_element = InitElement();
  UpdateCustomPropertyOnlyKeyframes(test_element.get(), base::String("test"));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  auto start_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  const auto* start_value =
      FindSampledCustomProperty(start_sample, base::String("--x"));
  ASSERT_NE(nullptr, start_value);
  EXPECT_TRUE(start_value->AsString().IsEqual("10px"));
  auto start_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, start_events.size());
  EXPECT_TRUE(start_events[0].send_start_event);

  auto end_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_value =
      FindSampledCustomProperty(end_sample, base::String("--x"));
  ASSERT_NE(nullptr, end_value);
  EXPECT_TRUE(end_value->AsString().IsEqual("20px"));
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);

  auto repeated_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* repeated_value =
      FindSampledCustomProperty(repeated_sample, base::String("--x"));
  ASSERT_NE(nullptr, repeated_value);
  EXPECT_TRUE(repeated_value->AsString().IsEqual("20px"));
  auto repeated_events =
      test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(repeated_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       RemovingCustomPropertyOnlyAnimationQueuesResetAndCancelForNewPipeline) {
  auto test_element = InitElement();
  UpdateCustomPropertyOnlyKeyframes(test_element.get(), base::String("test"));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  base::Vector<starlight::AnimationData> empty_animation_data;
  test_manager->SyncAnimationDataForNewPipeline(empty_animation_data);
  EXPECT_TRUE(test_manager->animations_map().empty());

  auto cleanup_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  EXPECT_TRUE(cleanup_sample.requires_base_style_rebuild);
  EXPECT_NE(std::find(cleanup_sample.custom_property_resets.begin(),
                      cleanup_sample.custom_property_resets.end(),
                      base::String("--x")),
            cleanup_sample.custom_property_resets.end());
  auto cancel_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, cancel_events.size());
  EXPECT_TRUE(cancel_events[0].send_cancel_event);
}

TEST_F(CSSKeyframeManagerTest,
       PausedSeekKeepsRequestedStyleAcrossDummyAndRealSamples) {
  for (bool has_previous_sample : {false, true}) {
    SCOPED_TRACE(has_previous_sample);
    auto test_element = InitElement();
    UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0, 1);
    auto test_manager = InitTestKeyframeManager(test_element.get());
    base::Vector<starlight::AnimationData> animation_data;
    animation_data.emplace_back(InitAnimationData(
        base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
        starlight::AnimationFillModeType::kBoth,
        starlight::AnimationDirectionType::kNormal,
        starlight::AnimationPlayStateType::kRunning));
    test_manager->SyncAnimationDataForNewPipeline(animation_data);
    auto animation = test_manager->animations_map().at(base::String("test"));
    if (has_previous_sample) {
      auto start_time = TimePointFromMs(1000);
      test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
      auto mid_time = TimePointFromMs(1500);
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
    }
    test_manager->TakePendingAnimationEventsForNewPipeline();
    animation->Pause();

    // Check both a first seek and a later seek with a populated sample cache.
    for (int requested_ms : {750, 250}) {
      SCOPED_TRACE(requested_ms);
      const auto reference_time = TimePointFromMs(5000);
      const auto requested_time =
          fml::TimeDelta::FromMilliseconds(requested_ms);
      animation->SeekTo(requested_time, reference_time);
      for (auto sample_time :
           {animation::Animation::GetAnimationDummyStartTime(),
            animation::Animation::GetAnimationDummyStartTime(),
            TimePointFromMs(8000)}) {
        auto sample =
            test_manager->CollectAnimationUpdatesForNewPipeline(sample_time);
        const auto* opacity = FindSampledStyle(sample, kPropertyIDOpacity);
        ASSERT_NE(nullptr, opacity);
        EXPECT_NEAR(requested_ms / 1000.0, opacity->AsNumber(), 0.001);
        EXPECT_EQ(reference_time, animation->pause_time());
        EXPECT_EQ(requested_time, animation->GetCurrentTime());
        EXPECT_EQ(animation::Animation::State::kPause, animation->GetState());
        EXPECT_TRUE(
            test_manager->TakePendingAnimationEventsForNewPipeline().empty());
      }
    }
  }
}

TEST_F(CSSKeyframeManagerTest, CSSResumeNotifiesInspectorInBothPipelines) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP() << "Inspector is disabled";
  }

  class RecordingObserver : public InspectorAnimationObserver {
   public:
    void OnAnimationUpdated(animation::Animation* animation) override {
      states.push_back(animation->GetState());
    }
    base::Vector<animation::Animation::State> states;
  };
  for (bool new_pipeline : {false, true}) {
    SCOPED_TRACE(new_pipeline);
    auto observer = std::make_shared<RecordingObserver>();
    manager->SetInspectorAnimationObserver(observer);
    auto test_element = InitElement();
    UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0, 1);
    auto test_manager = InitTestKeyframeManager(test_element.get());
    base::Vector<starlight::AnimationData> data;
    data.emplace_back(InitAnimationData(
        base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
        starlight::AnimationFillModeType::kBoth,
        starlight::AnimationDirectionType::kNormal,
        starlight::AnimationPlayStateType::kRunning));
    auto sync = [&]() {
      if (new_pipeline) {
        test_manager->SyncAnimationDataForNewPipeline(data);
      } else {
        test_manager->SetAnimationDataAndPlay(data);
      }
    };
    sync();
    EXPECT_TRUE(observer->states.empty());
    data[0].play_state = starlight::AnimationPlayStateType::kPaused;
    sync();
    data[0].play_state = starlight::AnimationPlayStateType::kRunning;
    sync();
    sync();
    ASSERT_EQ(2U, observer->states.size());
    EXPECT_EQ(animation::Animation::State::kPause, observer->states[0]);
    EXPECT_EQ(animation::Animation::State::kPlay, observer->states[1]);
  }
}

TEST_F(
    CSSKeyframeManagerTest,
    NewPipelineRemovalAndRebuildNotifyInspectorForRunningAndFinishedAnimations) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP() << "Inspector is disabled";
  }

  class RecordingObserver : public InspectorAnimationObserver {
   public:
    void OnAnimationCreated(animation::Animation* animation) override {
      created.push_back(animation->id());
    }
    void OnAnimationCanceled(animation::Animation* animation) override {
      canceled.push_back(animation->id());
    }
    base::Vector<int64_t> created;
    base::Vector<int64_t> canceled;
  };

  for (bool finished : {false, true}) {
    for (bool rebuild : {false, true}) {
      SCOPED_TRACE(finished);
      SCOPED_TRACE(rebuild);
      auto observer = std::make_shared<RecordingObserver>();
      manager->SetInspectorAnimationObserver(observer);
      auto test_element = InitElement();
      UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0, 1);
      auto test_manager = InitTestKeyframeManager(test_element.get());
      base::Vector<starlight::AnimationData> animation_data;
      animation_data.emplace_back(InitAnimationData(
          base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
          starlight::AnimationFillModeType::kBoth,
          starlight::AnimationDirectionType::kNormal,
          starlight::AnimationPlayStateType::kRunning));
      test_manager->SyncAnimationDataForNewPipeline(animation_data);
      ASSERT_EQ(1U, observer->created.size());
      const auto old_id = observer->created[0];
      auto start_time = TimePointFromMs(1000);
      test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
      if (finished) {
        auto end_time = TimePointFromMs(2500);
        test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
        ASSERT_EQ(animation::Animation::State::kStop,
                  test_manager->animations_map()
                      .at(base::String("test"))
                      ->GetState());
      }
      test_manager->TakePendingAnimationEventsForNewPipeline();
      EXPECT_TRUE(observer->canceled.empty());

      if (rebuild) {
        test_manager->SyncAnimationDataForNewPipeline(animation_data, true);
        ASSERT_EQ(2U, observer->created.size());
        EXPECT_NE(old_id, observer->created[1]);
      } else {
        base::Vector<starlight::AnimationData> empty_data;
        test_manager->SyncAnimationDataForNewPipeline(empty_data);
        EXPECT_TRUE(test_manager->animations_map().empty());
        EXPECT_EQ(1U, observer->created.size());
      }
      ASSERT_EQ(1U, observer->canceled.size());
      EXPECT_EQ(old_id, observer->canceled[0]);

      // Inspector removal is independent of page cancel event eligibility.
      auto events = test_manager->TakePendingAnimationEventsForNewPipeline();
      ASSERT_EQ(finished ? 0U : 1U, events.size());
      if (!finished) {
        EXPECT_TRUE(events[0].send_cancel_event);
      }
      test_element->DispatchAnimationEventsForNewPipeline(events);
      EXPECT_EQ(1U, observer->canceled.size());
    }
  }
}

TEST_F(CSSKeyframeManagerTest,
       PausedAnimationKeepsLastSampleWhenNextResolveUsesDummyTime) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto mid_time = fml::TimePoint::FromTicks(1500 * 1000 * 1000);
  auto mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* mid_opacity =
      FindSampledStyle(mid_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, mid_opacity);
  EXPECT_NEAR(0.5, mid_opacity->AsNumber(), 0.001);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  animation_data[0].play_state = starlight::AnimationPlayStateType::kPaused;
  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  auto paused_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  const auto* paused_opacity =
      FindSampledStyle(paused_sample, tasm::kPropertyIDOpacity);

  ASSERT_NE(nullptr, paused_opacity);
  EXPECT_NEAR(mid_opacity->AsNumber(), paused_opacity->AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       PausedAnimationRefreshesUnderlyingValueAtFrozenSampleTime) {
  auto test_element = InitElement();
  auto* base_style = test_element->computed_css_style();
  ASSERT_TRUE(base_style->SetValue(kPropertyIDOpacity,
                                   CSSValue(0.5f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  auto mid_time = TimePointFromMs(1500);
  auto mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* mid_opacity =
      FindSampledStyle(mid_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, mid_opacity);
  EXPECT_NEAR(0.25, mid_opacity->AsNumber(), 0.001);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  animation_data[0].play_state = starlight::AnimationPlayStateType::kPaused;
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  ASSERT_TRUE(base_style->ResetValue(kPropertyIDOpacity));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto refreshed_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  const auto* refreshed_opacity =
      FindSampledStyle(refreshed_sample, tasm::kPropertyIDOpacity);

  ASSERT_NE(nullptr, refreshed_opacity);
  EXPECT_NEAR(0.5, refreshed_opacity->AsNumber(), 0.001);
  EXPECT_TRUE(test_manager->TakePendingAnimationEventsForNewPipeline().empty());
}

TEST_F(CSSKeyframeManagerTest,
       PausedAnimationWithoutHistoryIgnoresDummyTimeResolve) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kPaused));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto animation = test_manager->animations_map()[base::String("test")];
  ASSERT_EQ(animation::Animation::State::kPause, animation->GetState());

  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  auto dummy_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);

  EXPECT_TRUE(dummy_sample.empty());
  EXPECT_EQ(animation->pause_time(), fml::TimePoint::Min());
  EXPECT_TRUE(test_manager->TakePendingAnimationEventsForNewPipeline().empty());
}

TEST_F(CSSKeyframeManagerTest,
       StoppedAnimationDoesNotEmitEventsAfterDummyResolve) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);

  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  auto dummy_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(dummy_events.empty());

  auto later_time = fml::TimePoint::FromTicks(2016 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(later_time);
  auto later_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(later_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineReplayFromStoppedAnimationClearsPauseTiming) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 4000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto animation = test_manager->animations_map()[base::String("test")];

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  animation->Pause();
  auto pause_time = fml::TimePoint::FromTicks(1500 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(pause_time);
  EXPECT_EQ(animation->pause_time(), pause_time);

  animation->Stop();
  auto updated_animation_data = animation_data;
  updated_animation_data[0].duration = 5000;
  test_manager->SyncAnimationDataForNewPipeline(updated_animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_EQ(animation.get(),
            test_manager->animations_map()[base::String("test")].get());
  EXPECT_EQ(animation->pause_time(), fml::TimePoint::Min());
  EXPECT_EQ(animation->total_paused_duration(), fml::TimeDelta::Zero());

  auto next_frame_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(next_frame_time);
  EXPECT_EQ(animation->total_paused_duration(), fml::TimeDelta::Zero());
}

TEST_F(CSSKeyframeManagerTest,
       FinishedAnimationKeepsSampledStyleForSameTimestampResolve) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_opacity =
      FindSampledStyle(end_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, end_opacity);
  EXPECT_NEAR(0.8, end_opacity->AsNumber(), 0.001);
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);

  auto repeated_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* repeated_opacity =
      FindSampledStyle(repeated_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, repeated_opacity);
  EXPECT_NEAR(end_opacity->AsNumber(), repeated_opacity->AsNumber(), 0.001);
  auto repeated_events =
      test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(repeated_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       ForwardsFillPersistsAfterStoppedAnimationLaterResolve) {
  auto test_element = InitElement();
  UpdateLeftKeyframes(test_element.get(), base::String("move"), "0px", "200px");
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("move"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kForwards,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = TimePointFromMs(2000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_left = FindSampledStyle(end_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, end_left);
  EXPECT_EQ(*end_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);
  ASSERT_TRUE(test_manager->animations_map().count(base::String("move")));
  EXPECT_EQ(animation::Animation::State::kStop,
            test_manager->animations_map()[base::String("move")]->GetState());

  auto later_time = TimePointFromMs(2016);
  auto later_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(later_time);
  const auto* later_left =
      FindSampledStyle(later_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, later_left);
  EXPECT_EQ(*later_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto later_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(later_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       CancelledRestartedForwardsAnimationPersistsSecondFinish) {
  auto test_element = InitElement();
  test_element->SetStyle(tasm::kPropertyIDLeft, lepus::Value("0px"));
  UpdateLeftKeyframes(test_element.get(), base::String("move"), "0px", "200px");
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("move"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kForwards,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto mid_time = TimePointFromMs(1500);
  auto mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* mid_left = FindSampledStyle(mid_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, mid_left);

  base::Vector<starlight::AnimationData> empty_animation_data;
  test_manager->SyncAnimationDataForNewPipeline(empty_animation_data);
  auto cleanup_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* cleanup_left =
      FindSampledStyle(cleanup_sample, tasm::kPropertyIDLeft);
  auto cleanup_reset_iter =
      std::find(cleanup_sample.property_resets.begin(),
                cleanup_sample.property_resets.end(), tasm::kPropertyIDLeft);
  EXPECT_TRUE(cleanup_left != nullptr ||
              cleanup_reset_iter != cleanup_sample.property_resets.end());
  if (cleanup_left != nullptr) {
    EXPECT_EQ(*cleanup_left, tasm::CSSValue(0, tasm::CSSValuePattern::PX));
    EXPECT_EQ(cleanup_reset_iter, cleanup_sample.property_resets.end());
  }
  auto cancel_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, cancel_events.size());
  EXPECT_TRUE(cancel_events[0].send_cancel_event);

  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  auto restart_time = TimePointFromMs(3000);
  test_manager->CollectAnimationUpdatesForNewPipeline(restart_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto second_end_time = TimePointFromMs(4000);
  auto second_end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(second_end_time);
  const auto* second_end_left =
      FindSampledStyle(second_end_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, second_end_left);
  EXPECT_EQ(*second_end_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto second_end_events =
      test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, second_end_events.size());
  EXPECT_TRUE(second_end_events[0].send_end_event);

  auto later_time = TimePointFromMs(4016);
  auto later_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(later_time);
  const auto* later_left =
      FindSampledStyle(later_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, later_left);
  EXPECT_EQ(*later_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto later_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(later_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       ForceRebuildUpdatesCurvesWithOrWithoutDurationChanges) {
  for (int duration : {2000, 3000}) {
    SCOPED_TRACE(duration);
    auto test_element = InitElement();
    auto test_manager = InitTestKeyframeManager(test_element.get());
    base::Vector<starlight::AnimationData> animation_data;
    animation_data.emplace_back(InitAnimationData(
        base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
        starlight::AnimationFillModeType::kBoth,
        starlight::AnimationDirectionType::kNormal,
        starlight::AnimationPlayStateType::kRunning));

    UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
    test_manager->SyncAnimationDataForNewPipeline(animation_data);
    auto initial = test_manager->animations_map().at(base::String("test"));
    ASSERT_NE(initial, nullptr);

    animation_data[0].duration = duration;
    UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.8, 0.9);
    test_manager->SyncAnimationDataForNewPipeline(animation_data, true);

    auto rebuilt = test_manager->animations_map().at(base::String("test"));
    ASSERT_EQ(rebuilt, initial);
    EXPECT_EQ(rebuilt->animation_data()->duration, duration);
    auto* model = rebuilt->keyframe_effect()->GetKeyframeModelByCurveType(
        animation::AnimationCurve::CurveType::OPACITY);
    ASSERT_NE(nullptr, model);
    auto* curve = static_cast<animation::KeyframedOpacityAnimationCurve*>(
        model->animation_curve());
    ASSERT_EQ(2U, curve->keyframes_.size());
    EXPECT_FLOAT_EQ(
        0.8f, static_cast<gfx::FloatKeyframe*>(curve->keyframes_.front().get())
                  ->Value());
    EXPECT_FLOAT_EQ(
        0.9f, static_cast<gfx::FloatKeyframe*>(curve->keyframes_.back().get())
                  ->Value());
  }
}

TEST_F(CSSKeyframeManagerTest,
       RebuildPreservesImperativeOriginAcrossExecutionBackends) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  target->imperative_animation_metadata_->RecordStart(
      ImperativeAnimationSource::kAnimateV2, data[0].name, data[0].name);

  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-in");
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  auto initial = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(initial, nullptr);
  EXPECT_EQ(initial->GetOrigin(), animation::Animation::Origin::kWebAnimation);

  // Finishing source tracking must not reclassify a retained animation when
  // its effect is rebuilt, including a round trip through the platform path.
  target->imperative_animation_metadata_->Finish(
      ImperativeAnimationSource::kAnimateV2, data[0].name);
  ASSERT_FALSE(target->HasImperativeAnimationMetadata(data[0].name));
  keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
  auto rebuilt = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(rebuilt, nullptr);
  EXPECT_EQ(rebuilt, initial);
  EXPECT_EQ(rebuilt->GetOrigin(), animation::Animation::Origin::kWebAnimation);

  UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
  keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
  ASSERT_EQ(keyframe_manager->animations_map().count(data[0].name), 0u);
  ASSERT_EQ(keyframe_manager->platform_animations_.count(data[0].name), 1u);
  keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
  ASSERT_EQ(keyframe_manager->platform_animations_.count(data[0].name), 1u);

  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-out");
  keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
  auto fallback = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(fallback, nullptr);
  EXPECT_EQ(fallback->GetOrigin(), animation::Animation::Origin::kWebAnimation);
  EXPECT_TRUE(keyframe_manager->platform_animations_.empty());
}

TEST_F(CSSKeyframeManagerTest, RoutesUsingInitiallyBoundAnimationEvents) {
  manager->GetConfig()->SetEnableFiberArch(true);
  for (const auto* type : {"bindEvent", "global-bindEvent", "worklet"}) {
    SCOPED_TRACE(type);
    for (const auto* event : {"animationend", "animationiteration"}) {
      SCOPED_TRACE(event);
      auto target = InitElement();
      auto animator = InitTestKeyframeManager(target.get());
      base::Vector<starlight::AnimationData> data(1);
      data[0].name = base::String("test");
      data[0].duration = 4000;
      data[0].iteration_count = 3;
      data[0].play_state = starlight::AnimationPlayStateType::kPaused;
      UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
      if (std::string(type) == "worklet") {
        target->SetWorkletEventHandler(
            event, "bindEvent", lepus::Value("worklet"),
            static_cast<runtime::MTSRuntime*>(nullptr));
      } else {
        target->SetJSEventHandler(event, type, "onAnimationEvent");
      }
      animator->SetAnimationDataAndPlay(data);
      const bool needs_iteration = std::string(event) == "animationiteration";
      EXPECT_EQ(animator->platform_animations_.empty(), needs_iteration);
      EXPECT_EQ(animator->animations_map().count(data[0].name),
                needs_iteration ? 1u : 0u);
    }
  }
}

TEST_F(CSSKeyframeManagerTest, PausedAnimationSurvivesBackendRoundTrip) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  auto animator = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 4000;
  data[0].iteration_count = 1;
  data[0].fill_mode = starlight::AnimationFillModeType::kBoth;
  data[0].play_state = starlight::AnimationPlayStateType::kRunning;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-in");
  animator->SyncAnimationDataForNewPipeline(data);
  auto logical = animator->animations_map().at(data[0].name);
  auto time = TimePointFromMs(10000);
  logical->SampleAt(time);
  logical->SendStartEvent();
  time = TimePointFromMs(11000);
  auto before = logical->SampleAt(time);
  ASSERT_TRUE(before.styles.count(kPropertyIDOpacity));

  data[0].play_state = starlight::AnimationPlayStateType::kPaused;
  animator->SyncAnimationDataForNewPipeline(data);
  const auto paused_time = logical->GetCurrentTime();
  EXPECT_GE(paused_time, fml::TimeDelta::FromMilliseconds(1000));
  auto sample_offset = paused_time;
  const auto expected_value =
      logical->keyframe_effect()
          ->GetKeyframeModelByCurveType(
              animation::AnimationCurve::CurveType::OPACITY)
          ->animation_curve()
          ->GetValue(sample_offset);

  UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
  animator->SyncAnimationDataForNewPipeline(data, true);
  ASSERT_TRUE(animator->animations_map().empty());
  auto& platform = animator->platform_animations_.at(data[0].name);
  EXPECT_EQ(platform.animation, logical);
  EXPECT_EQ(logical->keyframe_effect(), nullptr);
  EXPECT_EQ(logical->GetCurrentTime(), paused_time);
  const auto platform_executor = logical->executor();
  auto playback = logical->playback();
  ASSERT_NE(playback, nullptr);
  EXPECT_EQ(playback->event_state(),
            gfx::AnimationPlaybackState::EventState::kStarted);
  animator->ClearUTStatus();
  time = TimePointFromMs(20000);
  logical->DoFrame(time);  // A previously queued C++ tick must be inert.
  EXPECT_FALSE(animator->has_request_next_frame());

  // Rebuilding a native instance also retains the same clock and ownership.
  animator->SyncAnimationDataForNewPipeline(data, true);
  EXPECT_EQ(animator->platform_animations_.at(data[0].name).animation, logical);
  EXPECT_EQ(logical->executor(), platform_executor);
  EXPECT_EQ(logical->GetCurrentTime(), paused_time);

  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-in");
  animator->SyncAnimationDataForNewPipeline(data, true);
  EXPECT_EQ(animator->animations_map().at(data[0].name), logical);
  EXPECT_EQ(logical->GetState(), animation::Animation::State::kPause);
  EXPECT_EQ(logical->GetCurrentTime(), paused_time);
  EXPECT_FALSE(playback->IsCurrentExecutor(platform_executor));
  EXPECT_FALSE(playback->ClaimEvent(
      platform_executor, gfx::AnimationPlaybackState::EventState::kFinished));
  ASSERT_FALSE(target->platform_animation_commands_->empty());
  EXPECT_EQ(target->platform_animation_commands_->back().type,
            gfx::PlatformAnimationCommandType::kDetach);
  EXPECT_TRUE(animator->TakePendingAnimationEventsForNewPipeline().empty());

  // A styling pass can prime the replacement at dummy time while paused.
  auto primed =
      logical->SampleAt(animation::Animation::GetAnimationDummyStartTime());
  ASSERT_TRUE(primed.styles.count(kPropertyIDOpacity));
  EXPECT_EQ(primed.styles.at(kPropertyIDOpacity), expected_value);
  // Rebase into another timestamp epoch without recreating the animation.
  time = TimePointFromMs(50000);
  auto after = logical->SampleAt(time);
  ASSERT_TRUE(after.styles.count(kPropertyIDOpacity));
  EXPECT_EQ(after.styles.at(kPropertyIDOpacity), expected_value);
  EXPECT_FALSE(after.should_send_start_event);
  EXPECT_FALSE(after.should_send_end_event);
  EXPECT_EQ(after.iteration_events_due, 0);
  time = TimePointFromMs(90000);
  auto still_paused = logical->SampleAt(time);
  EXPECT_EQ(still_paused.styles, after.styles);
}

TEST_F(CSSKeyframeManagerTest,
       RunningPlatformFallbackRetainsElapsedIterations) {
  manager->GetConfig()->SetEnableFiberArch(true);
  auto target = InitElement();
  auto animator = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  data[0].iteration_count = 10;
  data[0].play_state = starlight::AnimationPlayStateType::kRunning;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
  animator->SyncAnimationDataForNewPipeline(data);
  auto logical = animator->platform_animations_.at(data[0].name).animation;
  auto playback = logical->playback();
  ASSERT_NE(playback, nullptr);
  playback->SetCurrentTime(fml::TimeDelta::FromMilliseconds(2500));
  EXPECT_TRUE(playback->ClaimEvent(
      logical->executor(), gfx::AnimationPlaybackState::EventState::kStarted));

  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-in");
  animator->SyncAnimationDataForNewPipeline(data, true);
  EXPECT_EQ(animator->animations_map().at(data[0].name), logical);
  EXPECT_GE(logical->GetCurrentTime(), fml::TimeDelta::FromMilliseconds(2500));
  auto time = TimePointFromMs(100000);
  auto sample = logical->SampleAt(time);
  ASSERT_TRUE(sample.styles.count(kPropertyIDOpacity));
  EXPECT_GE(time - logical->start_time(),
            fml::TimeDelta::FromMilliseconds(2500));
  EXPECT_FALSE(sample.should_send_start_event);
  EXPECT_FALSE(sample.should_send_end_event);
  EXPECT_EQ(sample.iteration_events_due, 0);
  EXPECT_TRUE(animator->TakePendingAnimationEventsForNewPipeline().empty());
}

TEST_F(CSSKeyframeManagerTest, LegacyStylingPrimesFallbackAtRetainedTime) {
  manager->GetConfig()->SetEnableFiberArch(true);
  for (bool paused : {false, true}) {
    auto target = InitElement();
    auto animator = InitTestKeyframeManager(target.get());
    base::Vector<starlight::AnimationData> data(1);
    data[0].name = base::String("test");
    data[0].duration = 4000;
    data[0].play_state = paused ? starlight::AnimationPlayStateType::kPaused
                                : starlight::AnimationPlayStateType::kRunning;
    UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
    animator->SetAnimationDataAndPlay(data);
    auto logical = animator->platform_animations_.at(data[0].name).animation;
    logical->playback()->SetCurrentTime(fml::TimeDelta::FromMilliseconds(1000));
    logical->playback()->ClaimEvent(
        logical->executor(), gfx::AnimationPlaybackState::EventState::kStarted);
    animator->ClearUTStatus();
    UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-in");
    animator->SetAnimationDataAndPlayInternal(data, true, true, false);
    EXPECT_EQ(animator->animations_map().at(data[0].name), logical);
    EXPECT_TRUE(animator->has_flush_animated_style());
    EXPECT_EQ(logical->GetState(), paused ? animation::Animation::State::kPause
                                          : animation::Animation::State::kPlay);
    ASSERT_TRUE(target->final_animator_map_.has_value());
    ASSERT_TRUE(target->final_animator_map_->count(kPropertyIDOpacity));
    EXPECT_GT(target->final_animator_map_->at(kPropertyIDOpacity).GetNumber(),
              0);
    EXPECT_EQ(target->platform_animation_commands_->back().type,
              gfx::PlatformAnimationCommandType::kDetach);
  }
}

TEST_F(CSSKeyframeManagerTest, PlatformFallbackDuringDelayDoesNotEmitEnd) {
  manager->GetConfig()->SetEnableFiberArch(true);
  auto target = InitElement();
  auto animator = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 4000;
  data[0].delay = 6000;
  data[0].iteration_count = 1;
  data[0].fill_mode = starlight::AnimationFillModeType::kBoth;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
  animator->SyncAnimationDataForNewPipeline(data);
  auto logical = animator->platform_animations_.at(data[0].name).animation;
  auto playback = logical->playback();
  playback->SetCurrentTime(fml::TimeDelta::FromMilliseconds(1000));
  // The existing native executor notifies start when it installs the effect.
  playback->ClaimEvent(logical->executor(),
                       gfx::AnimationPlaybackState::EventState::kStarted);
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-in");
  animator->SyncAnimationDataForNewPipeline(data, true);
  auto time = TimePointFromMs(100000);
  auto sample = logical->SampleAt(time);
  EXPECT_FALSE(sample.should_send_start_event);
  EXPECT_FALSE(sample.should_send_end_event);
  EXPECT_EQ(sample.iteration_events_due, 0);
  ASSERT_TRUE(sample.styles.count(kPropertyIDOpacity));
  EXPECT_DOUBLE_EQ(sample.styles.at(kPropertyIDOpacity).GetNumber(), 0);
}

TEST_F(CSSKeyframeManagerTest, UnavailableBackendKeepsOpacityInCore) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto capabilities = MakeIOSCapabilities();
  capabilities.backend = gfx::AnimationBackendType::kNone;
  routing_painting_context_->SetPlatformAnimationCapabilities(
      std::move(capabilities));
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.0, 1.0);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto running_animation = test_manager->animations_map()[base::String("test")];
  ASSERT_NE(running_animation, nullptr);
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::OPACITY),
            nullptr);
}

#if !OS_HARMONY
TEST_F(CSSKeyframeManagerTest, ExplicitNewAnimatorBypassesPlatformRouting) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  target->SetAttribute(base::String("enable-new-animator"), lepus::Value(true));
  target->ConsumeAllAttributes();
  auto animator = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  UpdateOpacityKeyframes(target.get(), data[0].name, 0.0, 1.0);

  animator->SyncAnimationDataForNewPipeline(data);

  ASSERT_EQ(animator->animations_map().count(data[0].name), 1u);
  EXPECT_TRUE(animator->platform_animations_.empty());
  EXPECT_FALSE(target->HasPendingPlatformAnimationCommands());
}
#endif

TEST_F(CSSKeyframeManagerTest,
       PlatformRoutingIgnoresDefaultNewAnimatorFlagAndHandoffsImmediately) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  struct RoutingCase {
    bool enable_new_animator;
    bool has_painting_node;
    int delay;
  };
  for (const auto& config :
       {RoutingCase{false, false, -100}, RoutingCase{true, true, 0}}) {
    SCOPED_TRACE(config.enable_new_animator);
    auto test_element = InitElement();
    test_element->enable_new_animator_ = config.enable_new_animator;
    test_element->has_painting_node_ = config.has_painting_node;
    ASSERT_TRUE(test_element->supports_platform_animation_routing());
    auto test_manager = InitTestKeyframeManager(test_element.get());
    UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.0, 1.0);
    base::Vector<starlight::AnimationData> animation_data;
    animation_data.emplace_back(
        InitAnimationData(base::String("test"), 1000, config.delay,
                          starlight::TimingFunctionData(), 1,
                          starlight::AnimationFillModeType::kBoth,
                          starlight::AnimationDirectionType::kNormal,
                          starlight::AnimationPlayStateType::kRunning));

    test_manager->SyncAnimationDataForNewPipeline(animation_data);
    EXPECT_FALSE(test_manager->animations_map().count(base::String("test")));
    EXPECT_FALSE(test_manager->has_request_next_frame());

    ASSERT_TRUE(test_element->HasPendingPlatformAnimationCommands());
    ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);
    const auto& command = test_element->platform_animation_commands_->front();
    EXPECT_EQ(command.type, gfx::PlatformAnimationCommandType::kHandoff);
    EXPECT_FALSE(test_element->can_be_layout_only_);
    ASSERT_EQ(command.properties.size(), 1u);
    EXPECT_EQ(command.properties.front().property,
              gfx::AnimationPropertyType::kOpacity);
  }
}

TEST_F(CSSKeyframeManagerTest,
       PlatformRoutingMaterializesMissingEndpointFromUnderlyingStyle) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  ASSERT_TRUE(test_element->computed_css_style()->SetValue(
      kPropertyIDOpacity, CSSValue(0.4f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0.0);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  EXPECT_FALSE(test_manager->animations_map().count(base::String("test")));
  ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);
  const auto& command = test_element->platform_animation_commands_->front();
  ASSERT_EQ(command.properties.size(), 1u);
  const auto& keyframes = command.properties.front().keyframes;
  ASSERT_EQ(keyframes.size(), 2u);
  EXPECT_EQ(keyframes.front()->timing_source(),
            gfx::Keyframe::TimingSource::kAnimation);
  EXPECT_FLOAT_EQ(
      static_cast<const gfx::FloatKeyframe*>(keyframes.front().get())->Value(),
      0.4f);
  EXPECT_FLOAT_EQ(
      static_cast<const gfx::FloatKeyframe*>(keyframes.back().get())->Value(),
      0.0f);
}

TEST_F(CSSKeyframeManagerTest,
       PlatformRoutingNormalizesKeyframesBeforeCapabilityCheck) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOutOfOrderOpacityKeyframes(test_element.get(), base::String("test"));
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);
  const auto& keyframes = test_element->platform_animation_commands_->front()
                              .properties.front()
                              .keyframes;
  ASSERT_EQ(keyframes.size(), 3u);
  EXPECT_DOUBLE_EQ(keyframes[0]->Offset(), 0.0);
  EXPECT_DOUBLE_EQ(keyframes[1]->Offset(), 0.5);
  EXPECT_DOUBLE_EQ(keyframes[2]->Offset(), 1.0);
}

TEST_F(CSSKeyframeManagerTest, CSSKeyframeTimingOverridesDefaultPerInterval) {
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  data[0].timing_func.timing_func = starlight::TimingFunctionType::kEaseIn;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-out");
  // Only the first interval has an explicit timing override.
  (*target->keyframes_map_)[data[0].name]->GetKeyframesContent().at(0.5)->erase(
      kPropertyIDAnimationTimingFunction);
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  auto anim = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(anim, nullptr);
  auto* curve = anim->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY)
                    ->animation_curve();
  auto sample = [&](double seconds) {
    auto time = fml::TimeDelta::FromSecondsF(seconds);
    return curve->GetValue(time).AsNumber();
  };
  auto ease_in = gfx::CubicBezierTimingFunction::CreatePreset(
      gfx::CubicBezierTimingFunction::EaseType::EASE_IN);
  auto ease_out = gfx::CubicBezierTimingFunction::CreatePreset(
      gfx::CubicBezierTimingFunction::EaseType::EASE_OUT);
  EXPECT_NEAR(sample(0.25), 0.5 * ease_out->GetValue(0.5), 1e-6);
  EXPECT_NEAR(sample(0.5), 0.5, 1e-6);
  EXPECT_NEAR(sample(0.75), 0.5 + 0.5 * ease_in->GetValue(0.5), 1e-6);

  // A duration-only update preserves the resolved interval timing objects and
  // must not apply animation-level easing to the timeline again.
  auto* typed_curve =
      static_cast<animation::KeyframedOpacityAnimationCurve*>(curve);
  const auto* default_timing = typed_curve->keyframes_[1]->timing_function();
  data[0].duration = 2000;
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  EXPECT_EQ(typed_curve->keyframes_[1]->timing_function(), default_timing);
  EXPECT_NEAR(sample(0.5), 0.5 * ease_out->GetValue(0.5), 1e-6);
  data[0].duration = 1000;

  data[0].timing_func.timing_func = starlight::TimingFunctionType::kLinear;
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  EXPECT_EQ(keyframe_manager->animations_map()[data[0].name], anim);
  EXPECT_NEAR(sample(0.25), 0.5 * ease_out->GetValue(0.5), 1e-6);
  EXPECT_NEAR(sample(0.75), 0.75, 1e-6);

  // The caller can mutate the same AnimationData instance before updating the
  // model. Compare against the model's snapshot, not the borrowed pointer.
  auto* model = anim->keyframe_effect()->GetKeyframeModelByCurveType(
      animation::AnimationCurve::CurveType::OPACITY);
  auto& model_data = anim->get_animation_data();
  model_data.timing_func.timing_func = starlight::TimingFunctionType::kEaseIn;
  model->UpdateAnimationData(&model_data);
  EXPECT_NEAR(sample(0.25), 0.5 * ease_out->GetValue(0.5), 1e-6);
  EXPECT_NEAR(sample(0.75), 0.5 + 0.5 * ease_in->GetValue(0.5), 1e-6);

  default_timing = typed_curve->keyframes_[1]->timing_function();
  model->UpdateAnimationData(nullptr);
  model->UpdateAnimationData(&model_data);
  EXPECT_EQ(typed_curve->keyframes_[1]->timing_function(), default_timing);
  EXPECT_NEAR(sample(0.75), 0.5 + 0.5 * ease_in->GetValue(0.5), 1e-6);

  model->UpdateAnimationData(nullptr);
  model_data.timing_func.timing_func = starlight::TimingFunctionType::kLinear;
  model->UpdateAnimationData(&model_data);
  EXPECT_NEAR(sample(0.25), 0.5 * ease_out->GetValue(0.5), 1e-6);
  EXPECT_NEAR(sample(0.75), 0.75, 1e-6);
}

TEST_F(CSSKeyframeManagerTest, EqualTimingValuesPreserveDistinctSources) {
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "linear");
  (*target->keyframes_map_)[data[0].name]->GetKeyframesContent().at(0.5)->erase(
      kPropertyIDAnimationTimingFunction);
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  auto anim = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(anim, nullptr);
  auto* curve = anim->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY)
                    ->animation_curve();
  auto sample = [&](double seconds) {
    auto time = fml::TimeDelta::FromSecondsF(seconds);
    return curve->GetValue(time).AsNumber();
  };
  EXPECT_NEAR(sample(0.25), 0.25, 1e-6);
  EXPECT_NEAR(sample(0.75), 0.75, 1e-6);

  // Both intervals initially use linear, but only the second follows changes
  // to the animation's timing. Updating a resolved value must retain its
  // source.
  for (auto timing : {starlight::TimingFunctionType::kEaseIn,
                      starlight::TimingFunctionType::kEaseOut}) {
    data[0].timing_func.timing_func = timing;
    keyframe_manager->SyncAnimationDataForNewPipeline(data);
    EXPECT_EQ(keyframe_manager->animations_map()[data[0].name], anim);
    auto expected = gfx::CreateTimingFunction(
        animation::ToGfxTimingFunctionData(data[0].timing_func));
    EXPECT_NEAR(sample(0.25), 0.25, 1e-6);
    EXPECT_NEAR(sample(0.75), 0.5 + 0.5 * expected->GetValue(0.5), 1e-6);
  }
}

TEST_F(CSSKeyframeManagerTest, SingleIntervalRoutesResolvedTimingOverride) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  data[0].timing_func.timing_func = starlight::TimingFunctionType::kEaseIn;
  for (const char* timing :
       {"ease-out", "linear", static_cast<const char*>(nullptr)}) {
    UpdateOpacityKeyframeTiming(target.get(), data[0].name, timing);
    (*target->keyframes_map_)[data[0].name]->GetKeyframesContent().erase(0.5);
    keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
    ASSERT_TRUE(keyframe_manager->animations_map().empty());
    ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
    const auto& command = target->platform_animation_commands_->back();
    auto resolved =
        gfx::CreateTimingFunction(command.animation_data.timing_func);
    auto expected =
        timing == nullptr
            ? gfx::CubicBezierTimingFunction::CreatePreset(
                  gfx::CubicBezierTimingFunction::EaseType::EASE_IN)
            : gfx::CubicBezierTimingFunction::CreatePreset(
                  gfx::CubicBezierTimingFunction::EaseType::EASE_OUT);
    EXPECT_NEAR(resolved->GetValue(0.25),
                timing != nullptr && std::string(timing) == "linear"
                    ? 0.25
                    : expected->GetValue(0.25),
                1e-6);
  }
}

TEST_F(CSSKeyframeManagerTest,
       DefaultNonlinearTimingFallsBackForMultipleIntervals) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  data[0].timing_func.timing_func = starlight::TimingFunctionType::kEaseIn;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, nullptr);
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  ASSERT_NE(keyframe_manager->animations_map()[data[0].name], nullptr);
  EXPECT_FALSE(target->HasPendingPlatformAnimationCommands());

  // Explicit linear on every interval overrides the default, so the platform
  // must receive linear effect timing rather than the unused ease-in value.
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "linear");
  keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
  EXPECT_TRUE(keyframe_manager->animations_map().empty());
  ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
  EXPECT_EQ(target->platform_animation_commands_->back()
                .animation_data.timing_func.timing_func,
            gfx::TimingFunctionType::kLinear);
}

TEST_F(CSSKeyframeManagerTest, SynthesizedStartUsesAnimationDefaultTiming) {
  auto target = InitElement();
  ASSERT_TRUE(target->computed_css_style()->SetValue(
      kPropertyIDOpacity, CSSValue(0.0f, CSSValuePattern::NUMBER)));
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  data[0].timing_func.timing_func = starlight::TimingFunctionType::kEaseIn;
  UpdateOpacityKeyframeTiming(target.get(), data[0].name, "ease-out");
  (*target->keyframes_map_)[data[0].name]->GetKeyframesContent().erase(0.0);
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  auto anim = keyframe_manager->animations_map()[data[0].name];
  ASSERT_NE(anim, nullptr);
  auto* curve = anim->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY)
                    ->animation_curve();
  auto time = fml::TimeDelta::FromSecondsF(0.25);
  auto ease_in = gfx::CubicBezierTimingFunction::CreatePreset(
      gfx::CubicBezierTimingFunction::EaseType::EASE_IN);
  EXPECT_NEAR(curve->GetValue(time).AsNumber(), 0.5 * ease_in->GetValue(0.5),
              1e-6);

  data[0].timing_func.timing_func = starlight::TimingFunctionType::kLinear;
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  EXPECT_EQ(keyframe_manager->animations_map()[data[0].name], anim);
  time = fml::TimeDelta::FromSecondsF(0.25);
  EXPECT_NEAR(curve->GetValue(time).AsNumber(), 0.25, 1e-6);
  time = fml::TimeDelta::FromSecondsF(0.75);
  auto ease_out = gfx::CubicBezierTimingFunction::CreatePreset(
      gfx::CubicBezierTimingFunction::EaseType::EASE_OUT);
  EXPECT_NEAR(curve->GetValue(time).AsNumber(),
              0.5 + 0.5 * ease_out->GetValue(0.5), 1e-6);
}

TEST_F(CSSKeyframeManagerTest, DifferentPropertyTimingsKeepWholeEffectInCore) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  data[0].timing_func.timing_func = starlight::TimingFunctionType::kEaseIn;
  auto keyframes = lepus::Dictionary::Create();
  auto from = lepus::Dictionary::Create();
  from->SetValue("opacity", lepus::Value(0.0));
  from->SetValue("animation-timing-function", lepus::Value("ease-out"));
  keyframes->SetValue("0", lepus::Value(from));
  auto to = lepus::Dictionary::Create();
  to->SetValue("opacity", lepus::Value(1.0));
  to->SetValue("transform", lepus::Value("translateX(100px)"));
  keyframes->SetValue("100", lepus::Value(to));
  starlight::CSSStyleUtils::UpdateCSSKeyframes(
      *target->keyframes_map_, data[0].name, lepus::Value(keyframes),
      target->element_manager()->GetCSSParserConfigs());
  keyframe_manager->SyncAnimationDataForNewPipeline(data);
  ASSERT_NE(keyframe_manager->animations_map()[data[0].name], nullptr);
  EXPECT_FALSE(target->HasPendingPlatformAnimationCommands());
}

TEST_F(CSSKeyframeManagerTest, RebuildUpdatesAndRemovesExplicitKeyframeTiming) {
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data(1);
  data[0].name = base::String("test");
  data[0].duration = 1000;
  for (const char* timing : {static_cast<const char*>(nullptr), "ease-in",
                             "ease-out", static_cast<const char*>(nullptr)}) {
    UpdateOpacityKeyframeTiming(target.get(), data[0].name, timing);
    keyframe_manager->SyncAnimationDataForNewPipeline(data, true);
    auto animation = keyframe_manager->animations_map()[data[0].name];
    ASSERT_NE(animation, nullptr);
    auto* model = animation->keyframe_effect()->GetKeyframeModelByCurveType(
        animation::AnimationCurve::CurveType::OPACITY);
    ASSERT_NE(model, nullptr);
    auto* curve = static_cast<animation::KeyframedOpacityAnimationCurve*>(
        model->animation_curve());
    ASSERT_EQ(curve->keyframes_.size(), 3u);
    for (const auto& frame : curve->keyframes_) {
      ASSERT_NE(frame->timing_function(), nullptr);
      if (timing == nullptr) {
        EXPECT_EQ(frame->timing_function()->GetType(),
                  gfx::TimingFunction::Type::LINEAR);
        EXPECT_DOUBLE_EQ(frame->timing_function()->GetValue(0.25), 0.25);
      } else {
        auto expected = gfx::CubicBezierTimingFunction::CreatePreset(
            std::string(timing) == "ease-in"
                ? gfx::CubicBezierTimingFunction::EaseType::EASE_IN
                : gfx::CubicBezierTimingFunction::EaseType::EASE_OUT);
        EXPECT_DOUBLE_EQ(frame->timing_function()->GetValue(0.25),
                         expected->GetValue(0.25));
      }
    }
  }
}

TEST_F(CSSKeyframeManagerTest,
       UnsupportedColorPropertiesKeepTheEntireEffectInCore) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateRasterColorKeyframes(test_element.get(), base::String("test"));
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto running_animation = test_manager->animations_map()[base::String("test")];
  ASSERT_NE(running_animation, nullptr);
  EXPECT_FALSE(test_element->HasPendingPlatformAnimationCommands());
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::OPACITY),
            nullptr);
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::BGCOLOR),
            nullptr);
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::TEXTCOLOR),
            nullptr);
}

TEST_F(CSSKeyframeManagerTest,
       AnimationWithOpacityAndLayoutFallsBackEntirelyToNewAnimator) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityAndLeftKeyframes(test_element.get(), base::String("test"));
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  auto running_animation = test_manager->animations_map()[base::String("test")];
  ASSERT_NE(running_animation, nullptr);
  EXPECT_FALSE(test_element->HasPendingPlatformAnimationCommands());
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::OPACITY),
            nullptr);
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::LEFT),
            nullptr);
}

TEST_F(CSSKeyframeManagerTest,
       AnimationWithOpacityAndTransformFallsBackEntirelyToNewAnimator) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto capabilities = MakeIOSCapabilities();
  capabilities.properties.erase(
      std::remove_if(capabilities.properties.begin(),
                     capabilities.properties.end(),
                     [](const auto& capability) {
                       return capability.property ==
                              gfx::AnimationPropertyType::kTransform;
                     }),
      capabilities.properties.end());
  routing_painting_context_->SetPlatformAnimationCapabilities(
      std::move(capabilities));
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityAndTransformKeyframes(test_element.get(), base::String("test"));
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto running_animation = test_manager->animations_map()[base::String("test")];
  ASSERT_NE(running_animation, nullptr);
  EXPECT_FALSE(test_element->HasPendingPlatformAnimationCommands());
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::OPACITY),
            nullptr);
  EXPECT_NE(running_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::TRANSFORM),
            nullptr);
}

TEST_F(CSSKeyframeManagerTest,
       AnimationWithOpacityAndTransformRoutesEntirelyToIOS) {
  manager->GetConfig()->SetEnableFiberArch(true);
  routing_painting_context_->SetPlatformAnimationCapabilities(
      MakeIOSCapabilities());
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityAndTransformKeyframes(test_element.get(), base::String("test"));
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  EXPECT_FALSE(test_manager->animations_map().count(base::String("test")));
  ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);
  const auto& command = test_element->platform_animation_commands_->front();
  ASSERT_EQ(command.properties.size(), 2u);
  EXPECT_TRUE(std::any_of(command.properties.begin(), command.properties.end(),
                          [](const auto& property) {
                            return property.property ==
                                   gfx::AnimationPropertyType::kOpacity;
                          }));
  EXPECT_TRUE(std::any_of(command.properties.begin(), command.properties.end(),
                          [](const auto& property) {
                            return property.property ==
                                   gfx::AnimationPropertyType::kTransform;
                          }));
}

TEST_F(CSSKeyframeManagerTest,
       PercentageTransformRoutesWithTypedUnitsPreserved) {
  manager->GetConfig()->SetEnableFiberArch(true);
  auto target = InitElement();
  auto animator = InitTestKeyframeManager(target.get());
  UpdateOpacityAndTransformKeyframes(target.get(), base::String("test"),
                                     "translate(50%, 25%)");
  base::Vector<starlight::AnimationData> data;
  data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  animator->SyncAnimationDataForNewPipeline(data);

  EXPECT_FALSE(animator->animations_map().count(base::String("test")));
  ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
  const auto& properties =
      target->platform_animation_commands_->front().properties;
  const auto transform = std::find_if(
      properties.begin(), properties.end(), [](const auto& property) {
        return property.property == gfx::AnimationPropertyType::kTransform;
      });
  ASSERT_NE(transform, properties.end());
  const auto* end = static_cast<const gfx::TransformKeyframe*>(
      transform->keyframes.back().get());
  ASSERT_TRUE(end->HasResolvedValue());
  const auto& operations = end->ResolvedValue().GetOperations();
  ASSERT_EQ(operations.size(), 1u);
  EXPECT_EQ(operations[0].translate.x.unit, gfx::LengthUnit::kPercent);
  EXPECT_FLOAT_EQ(operations[0].translate.x.value, 50.0f);
  EXPECT_EQ(operations[0].translate.y.unit, gfx::LengthUnit::kPercent);
  EXPECT_FLOAT_EQ(operations[0].translate.y.value, 25.0f);
}

TEST_F(CSSKeyframeManagerTest, RoutedKeyframeUpdatesReuseOrRebuildPayload) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.0, 1.0);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);

  // Explicit endpoints do not depend on changes to the base opacity.
  tasm::StyleMap base_styles{
      {tasm::kPropertyIDOpacity,
       tasm::CSSValue(0.4f, tasm::CSSValuePattern::NUMBER)}};
  test_manager->SyncAnimationDataForNewPipeline(animation_data, false,
                                                &base_styles);
  ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);

  const auto initial_keyframe =
      test_element->platform_animation_commands_->front()
          .properties.front()
          .keyframes.front();
  const auto initial_id =
      test_element->platform_animation_commands_->front().animation_id;
  EXPECT_FALSE(
      test_element->platform_animation_commands_->front().reuse_keyframes);
  animation_data[0].play_state = starlight::AnimationPlayStateType::kPaused;
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_EQ(test_element->platform_animation_commands_->size(), 2u);
  const auto& pause_update = test_element->platform_animation_commands_->back();
  EXPECT_EQ(pause_update.type, gfx::PlatformAnimationCommandType::kUpdate);
  EXPECT_EQ(pause_update.animation_data.play_state,
            gfx::AnimationPlayStateType::kPaused);
  EXPECT_FALSE(test_manager->animations_map().count(base::String("test")));
  EXPECT_TRUE(pause_update.reuse_keyframes);
  EXPECT_EQ(pause_update.properties.front().keyframes.front(),
            initial_keyframe);
  EXPECT_EQ(pause_update.animation_id, initial_id);
  EXPECT_EQ(pause_update.generation, 2u);

  // Forced rebuilds replace the payload whether play-state changes or not.
  uint64_t generation = 2;
  for (auto play_state : {starlight::AnimationPlayStateType::kPaused,
                          starlight::AnimationPlayStateType::kRunning}) {
    SCOPED_TRACE(static_cast<int>(play_state));
    animation_data[0].play_state = play_state;
    UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.1, 0.9);
    test_manager->SyncAnimationDataForNewPipeline(animation_data, true);
    ++generation;
    ASSERT_EQ(test_element->platform_animation_commands_->size(), generation);
    EXPECT_FALSE(test_manager->animations_map().count(base::String("test")));
    const auto& rebuilt = test_element->platform_animation_commands_->back();
    EXPECT_EQ(rebuilt.type, gfx::PlatformAnimationCommandType::kUpdate);
    EXPECT_FALSE(rebuilt.reuse_keyframes);
    EXPECT_EQ(rebuilt.animation_id, initial_id);
    EXPECT_EQ(rebuilt.generation, generation);
    ASSERT_EQ(rebuilt.properties.size(), 1u);
    const auto& keyframes = rebuilt.properties.front().keyframes;
    ASSERT_EQ(keyframes.size(), 2u);
    EXPECT_NE(keyframes.front(), initial_keyframe);
    EXPECT_FLOAT_EQ(
        static_cast<const gfx::FloatKeyframe*>(keyframes.front().get())
            ->Value(),
        0.1f);
    EXPECT_FLOAT_EQ(
        static_cast<const gfx::FloatKeyframe*>(keyframes.back().get())->Value(),
        0.9f);
  }
}

TEST_F(CSSKeyframeManagerTest,
       PlatformEndpointsRefreshOnlyWhenUnderlyingValuesChange) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto target = InitElement();
  auto keyframe_manager = InitTestKeyframeManager(target.get());
  UpdateToOnlyOpacityKeyframes(target.get(), base::String("test"), 1.0);
  base::Vector<starlight::AnimationData> data{InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning)};
  tasm::StyleMap base_styles{
      {tasm::kPropertyIDOpacity,
       tasm::CSSValue(0.2f, tasm::CSSValuePattern::NUMBER)}};
  keyframe_manager->SyncAnimationDataForNewPipeline(data, false, &base_styles);
  ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
  auto previous_keyframe = target->platform_animation_commands_->back()
                               .properties.front()
                               .keyframes.front();
  const auto animation_id =
      target->platform_animation_commands_->back().animation_id;
  EXPECT_FLOAT_EQ(
      static_cast<const gfx::FloatKeyframe*>(previous_keyframe.get())->Value(),
      0.2f);

  using PlayState = starlight::AnimationPlayStateType;
  struct Update {
    PlayState play_state;
    std::optional<float> opacity;
    bool expect_command;
    bool reuse_keyframes;
  };
  const Update updates[] = {
      // Only pause.
      {PlayState::kPaused, 0.2f, true, true},
      // Resume and change base value.
      {PlayState::kRunning, 0.4f, true, false},
      // Only change base value.
      {PlayState::kRunning, 0.6f, true, false},
      // Restore default.
      {PlayState::kRunning, std::nullopt, true, false},
      // Explicit default is equivalent.
      {PlayState::kPaused, 1.0f, true, true},
      // Only unrelated style changes.
      {PlayState::kPaused, 1.0f, false, true},
  };
  int step = 0;
  for (const auto& update : updates) {
    SCOPED_TRACE(step);
    data[0].play_state = update.play_state;
    base_styles.clear();
    if (update.opacity.has_value()) {
      base_styles.emplace(
          tasm::kPropertyIDOpacity,
          tasm::CSSValue(*update.opacity, tasm::CSSValuePattern::NUMBER));
    }
    // This animation does not depend on background-color.
    base_styles.emplace(tasm::kPropertyIDBackgroundColor,
                        tasm::CSSValue(++step, tasm::CSSValuePattern::NUMBER));
    const auto previous_command_count =
        target->platform_animation_commands_->size();
    keyframe_manager->SyncAnimationDataForNewPipeline(data, false,
                                                      &base_styles);
    ASSERT_EQ(target->platform_animation_commands_->size(),
              previous_command_count + (update.expect_command ? 1u : 0u));
    if (!update.expect_command) {
      continue;
    }
    const auto& command = target->platform_animation_commands_->back();
    EXPECT_EQ(command.type, gfx::PlatformAnimationCommandType::kUpdate);
    EXPECT_EQ(command.animation_id, animation_id);
    EXPECT_EQ(command.reuse_keyframes, update.reuse_keyframes);
    ASSERT_EQ(command.properties.size(), 1u);
    ASSERT_EQ(command.properties.front().keyframes.size(), 2u);
    const auto& endpoint = command.properties.front().keyframes.front();
    EXPECT_EQ(endpoint == previous_keyframe, update.reuse_keyframes);
    EXPECT_FLOAT_EQ(
        static_cast<const gfx::FloatKeyframe*>(endpoint.get())->Value(),
        update.opacity.value_or(1.0f));
    previous_keyframe = endpoint;
  }
}

TEST_F(CSSKeyframeManagerTest, RemovingRoutedKeyframeQueuesPlatformCancelOnly) {
  manager->GetConfig()->SetEnableFiberArch(true);
  manager->GetConfig()->SetEnableRasterAnimation(true);
  auto test_element = InitElement();
  test_element->has_painting_node_ = true;
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.0, 1.0);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  ASSERT_EQ(test_element->platform_animation_commands_->size(), 1u);
  const auto animation_id =
      test_element->platform_animation_commands_->front().animation_id;

  animation_data.clear();
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_EQ(test_element->platform_animation_commands_->size(), 2u);
  const auto& cancel = test_element->platform_animation_commands_->back();
  EXPECT_EQ(cancel.type, gfx::PlatformAnimationCommandType::kCancel);
  EXPECT_EQ(cancel.animation_id, animation_id);
  EXPECT_TRUE(test_manager->TakePendingAnimationEventsForNewPipeline().empty());
}

TEST_F(CSSKeyframeManagerTest, UpdateAndFlushAnimatedStyle) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());

  auto id = lynx::tasm::CSSPropertyID::kPropertyIDLeft;
  lynx::tasm::StyleMap test_map;
  lynx::tasm::CSSParserConfigs configs;
  auto impl = lepus::Value("100px");
  lynx::tasm::UnitHandler::Process(id, impl, test_map, configs);
  const auto& final_map = *test_element->final_animator_map_;

  bool update_flag = final_map.empty();
  EXPECT_TRUE(update_flag);

  auto [flush_flag, has_pending] = test_element->FlushAnimatedStyle();
  EXPECT_FALSE(flush_flag);

  test_manager->UpdateFinalStyleMap(test_map);
  update_flag = test_element->final_animator_map_->empty();
  EXPECT_FALSE(update_flag);

  std::tie(flush_flag, has_pending) = test_element->FlushAnimatedStyle();
  EXPECT_TRUE(flush_flag);
}

TEST_F(CSSKeyframeManagerTest, SetNeedsAnimationStyleRecalc) {
  auto test_animation = InitTestAnimation();
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  test_manager->SetNeedsAnimationStyleRecalc(test_animation->name());
  const auto& final_map = *test_element->final_animator_map_;
  bool update_flag = final_map.empty();
  EXPECT_TRUE(update_flag);
}

TEST_F(CSSKeyframeManagerTest, GetLayoutPropertyTypeSet) {
  auto test_set = animation::GetLayoutPropertyTypeSet();
  static const base::NoDestructor<
      std::unordered_set<starlight::AnimationPropertyType>>
      base_set({starlight::AnimationPropertyType::kWidth,
                starlight::AnimationPropertyType::kHeight,
                starlight::AnimationPropertyType::kTop,
                starlight::AnimationPropertyType::kLeft,
                starlight::AnimationPropertyType::kRight,
                starlight::AnimationPropertyType::kBottom,
                starlight::AnimationPropertyType::kBorderLeftWidth,
                starlight::AnimationPropertyType::kBorderRightWidth,
                starlight::AnimationPropertyType::kBorderTopWidth,
                starlight::AnimationPropertyType::kBorderBottomWidth,
                starlight::AnimationPropertyType::kPaddingLeft,
                starlight::AnimationPropertyType::kPaddingRight,
                starlight::AnimationPropertyType::kPaddingTop,
                starlight::AnimationPropertyType::kPaddingBottom,
                starlight::AnimationPropertyType::kMarginLeft,
                starlight::AnimationPropertyType::kMarginRight,
                starlight::AnimationPropertyType::kMarginTop,
                starlight::AnimationPropertyType::kMarginBottom,
                starlight::AnimationPropertyType::kMaxWidth,
                starlight::AnimationPropertyType::kMinWidth,
                starlight::AnimationPropertyType::kMaxHeight,
                starlight::AnimationPropertyType::kMinHeight,
                starlight::AnimationPropertyType::kFlexBasis});
  EXPECT_EQ(test_set, *base_set);
}

TEST_F(CSSKeyframeManagerTest, GetLayoutCurveTypeSet) {
  auto test_set = animation::GetLayoutCurveTypeSet();
  static const base::NoDestructor<
      std::unordered_set<animation::AnimationCurve::CurveType>>
      base_set({animation::AnimationCurve::CurveType::LEFT,
                animation::AnimationCurve::CurveType::RIGHT,
                animation::AnimationCurve::CurveType::TOP,
                animation::AnimationCurve::CurveType::BOTTOM,
                animation::AnimationCurve::CurveType::HEIGHT,
                animation::AnimationCurve::CurveType::WIDTH,
                animation::AnimationCurve::CurveType::MAX_WIDTH,
                animation::AnimationCurve::CurveType::MIN_WIDTH,
                animation::AnimationCurve::CurveType::MAX_HEIGHT,
                animation::AnimationCurve::CurveType::MIN_HEIGHT,
                animation::AnimationCurve::CurveType::PADDING_LEFT,
                animation::AnimationCurve::CurveType::PADDING_RIGHT,
                animation::AnimationCurve::CurveType::PADDING_TOP,
                animation::AnimationCurve::CurveType::PADDING_BOTTOM,
                animation::AnimationCurve::CurveType::MARGIN_LEFT,
                animation::AnimationCurve::CurveType::MARGIN_RIGHT,
                animation::AnimationCurve::CurveType::MARGIN_TOP,
                animation::AnimationCurve::CurveType::MARGIN_BOTTOM,
                animation::AnimationCurve::CurveType::BORDER_LEFT_WIDTH,
                animation::AnimationCurve::CurveType::BORDER_RIGHT_WIDTH,
                animation::AnimationCurve::CurveType::BORDER_TOP_WIDTH,
                animation::AnimationCurve::CurveType::BORDER_BOTTOM_WIDTH,
                animation::AnimationCurve::CurveType::FLEX_BASIS});
  EXPECT_EQ(test_set, *base_set);
}

TEST_F(CSSKeyframeManagerTest, GetPropertyIDToAnimationPropertyTypeMap) {
  auto test_map = animation::GetPropertyIDToAnimationPropertyTypeMap();
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      base_map({
          {tasm::kPropertyIDLeft, starlight::AnimationPropertyType::kLeft},
          {tasm::kPropertyIDTop, starlight::AnimationPropertyType::kTop},
          {tasm::kPropertyIDRight, starlight::AnimationPropertyType::kRight},
          {tasm::kPropertyIDBottom, starlight::AnimationPropertyType::kBottom},
          {tasm::kPropertyIDWidth, starlight::AnimationPropertyType::kWidth},
          {tasm::kPropertyIDHeight, starlight::AnimationPropertyType::kHeight},
          {tasm::kPropertyIDOpacity,
           starlight::AnimationPropertyType::kOpacity},
          {tasm::kPropertyIDBackgroundColor,
           starlight::AnimationPropertyType::kBackgroundColor},
          {tasm::kPropertyIDColor, starlight::AnimationPropertyType::kColor},
          {tasm::kPropertyIDMaxWidth,
           starlight::AnimationPropertyType::kMaxWidth},
          {tasm::kPropertyIDMinWidth,
           starlight::AnimationPropertyType::kMinWidth},
          {tasm::kPropertyIDMaxHeight,
           starlight::AnimationPropertyType::kMaxHeight},
          {tasm::kPropertyIDMinHeight,
           starlight::AnimationPropertyType::kMinHeight},
          {tasm::kPropertyIDMarginLeft,
           starlight::AnimationPropertyType::kMarginLeft},
          {tasm::kPropertyIDMarginRight,
           starlight::AnimationPropertyType::kMarginRight},
          {tasm::kPropertyIDMarginTop,
           starlight::AnimationPropertyType::kMarginTop},
          {tasm::kPropertyIDMarginBottom,
           starlight::AnimationPropertyType::kMarginBottom},
          {tasm::kPropertyIDPaddingLeft,
           starlight::AnimationPropertyType::kPaddingLeft},
          {tasm::kPropertyIDPaddingRight,
           starlight::AnimationPropertyType::kPaddingRight},
          {tasm::kPropertyIDPaddingTop,
           starlight::AnimationPropertyType::kPaddingTop},
          {tasm::kPropertyIDPaddingBottom,
           starlight::AnimationPropertyType::kPaddingBottom},
          {tasm::kPropertyIDBorderLeftWidth,
           starlight::AnimationPropertyType::kBorderLeftWidth},
          {tasm::kPropertyIDBorderRightWidth,
           starlight::AnimationPropertyType::kBorderRightWidth},
          {tasm::kPropertyIDBorderTopWidth,
           starlight::AnimationPropertyType::kBorderTopWidth},
          {tasm::kPropertyIDBorderBottomWidth,
           starlight::AnimationPropertyType::kBorderBottomWidth},
          {tasm::kPropertyIDBorderLeftColor,
           starlight::AnimationPropertyType::kBorderLeftColor},
          {tasm::kPropertyIDBorderRightColor,
           starlight::AnimationPropertyType::kBorderRightColor},
          {tasm::kPropertyIDBorderTopColor,
           starlight::AnimationPropertyType::kBorderTopColor},
          {tasm::kPropertyIDBorderBottomColor,
           starlight::AnimationPropertyType::kBorderBottomColor},
          {tasm::kPropertyIDFlexGrow,
           starlight::AnimationPropertyType::kFlexGrow},
          {tasm::kPropertyIDFlexBasis,
           starlight::AnimationPropertyType::kFlexBasis},
          {tasm::kPropertyIDFilter, starlight::AnimationPropertyType::kFilter},
          {tasm::kPropertyIDBoxShadow,
           starlight::AnimationPropertyType::kBoxShadow},
          {tasm::kPropertyIDOffsetDistance,
           starlight::AnimationPropertyType::kOffsetDistance},
          {tasm::kPropertyIDTransform,
           starlight::AnimationPropertyType::kTransform},
          {tasm::kPropertyIDBackgroundPosition,
           starlight::AnimationPropertyType::kBackgroundPosition},
          {tasm::kPropertyIDTransformOrigin,
           starlight::AnimationPropertyType::kTransformOrigin},
          {tasm::kPropertyIDVisibility,
           starlight::AnimationPropertyType::kVisibility},
      });
  EXPECT_EQ(test_map, *base_map);
}

TEST_F(CSSKeyframeManagerTest, GetAnimatablePropertyIDSet) {
  auto test_set = animation::GetAnimatablePropertyIDSet();
  static const base::NoDestructor<std::unordered_set<tasm::CSSPropertyID>>
      base_set({
          tasm::kPropertyIDTop,
          tasm::kPropertyIDLeft,
          tasm::kPropertyIDRight,
          tasm::kPropertyIDBottom,
          tasm::kPropertyIDWidth,
          tasm::kPropertyIDHeight,
          tasm::kPropertyIDBackgroundColor,
          tasm::kPropertyIDColor,
          tasm::kPropertyIDOpacity,
          tasm::kPropertyIDBorderLeftColor,
          tasm::kPropertyIDBorderRightColor,
          tasm::kPropertyIDBorderTopColor,
          tasm::kPropertyIDBorderBottomColor,
          tasm::kPropertyIDBorderLeftWidth,
          tasm::kPropertyIDBorderRightWidth,
          tasm::kPropertyIDBorderTopWidth,
          tasm::kPropertyIDBorderBottomWidth,
          tasm::kPropertyIDPaddingLeft,
          tasm::kPropertyIDPaddingRight,
          tasm::kPropertyIDPaddingTop,
          tasm::kPropertyIDPaddingBottom,
          tasm::kPropertyIDMarginLeft,
          tasm::kPropertyIDMarginRight,
          tasm::kPropertyIDMarginTop,
          tasm::kPropertyIDMarginBottom,
          tasm::kPropertyIDMaxWidth,
          tasm::kPropertyIDMinWidth,
          tasm::kPropertyIDMaxHeight,
          tasm::kPropertyIDMinHeight,
          tasm::kPropertyIDFlexGrow,
          tasm::kPropertyIDFlexBasis,
          tasm::kPropertyIDTransform,
          tasm::kPropertyIDFilter,
          tasm::kPropertyIDBoxShadow,
          tasm::kPropertyIDOffsetDistance,
          tasm::kPropertyIDBackgroundPosition,
          tasm::kPropertyIDTransformOrigin,
          tasm::kPropertyIDVisibility,
      });
  EXPECT_EQ(test_set, *base_set);
  bool test_flag = animation::IsAnimatableProperty(tasm::kPropertyIDOpacity);
  bool base_flag = *base_set->find(tasm::kPropertyIDOpacity);
  EXPECT_EQ(test_flag, base_flag);
}

TEST_F(CSSKeyframeManagerTest,
       GetPolymericPropertyIDToAnimationPropertyTypeMap) {
  auto test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kBorderWidth);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyBorderWidthMap({
          {tasm::kPropertyIDBorderTopWidth,
           starlight::AnimationPropertyType::kBorderTopWidth},
          {tasm::kPropertyIDBorderLeftWidth,
           starlight::AnimationPropertyType::kBorderLeftWidth},
          {tasm::kPropertyIDBorderRightWidth,
           starlight::AnimationPropertyType::kBorderRightWidth},
          {tasm::kPropertyIDBorderBottomWidth,
           starlight::AnimationPropertyType::kBorderBottomWidth},
      });
  EXPECT_EQ(test_map, *kIDPropertyBorderWidthMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kBorderColor);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyBorderColorMap({
          {tasm::kPropertyIDBorderTopColor,
           starlight::AnimationPropertyType::kBorderTopColor},
          {tasm::kPropertyIDBorderLeftColor,
           starlight::AnimationPropertyType::kBorderLeftColor},
          {tasm::kPropertyIDBorderRightColor,
           starlight::AnimationPropertyType::kBorderRightColor},
          {tasm::kPropertyIDBorderBottomColor,
           starlight::AnimationPropertyType::kBorderBottomColor},
      });
  EXPECT_EQ(test_map, *kIDPropertyBorderColorMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kMargin);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyMarginMap({
          {tasm::kPropertyIDMarginTop,
           starlight::AnimationPropertyType::kMarginTop},
          {tasm::kPropertyIDMarginLeft,
           starlight::AnimationPropertyType::kMarginLeft},
          {tasm::kPropertyIDMarginRight,
           starlight::AnimationPropertyType::kMarginRight},
          {tasm::kPropertyIDMarginBottom,
           starlight::AnimationPropertyType::kMarginBottom},
      });
  EXPECT_EQ(test_map, *kIDPropertyMarginMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kPadding);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyPaddingMap({
          {tasm::kPropertyIDPaddingTop,
           starlight::AnimationPropertyType::kPaddingTop},
          {tasm::kPropertyIDPaddingLeft,
           starlight::AnimationPropertyType::kPaddingLeft},
          {tasm::kPropertyIDPaddingRight,
           starlight::AnimationPropertyType::kPaddingRight},
          {tasm::kPropertyIDPaddingBottom,
           starlight::AnimationPropertyType::kPaddingBottom},
      });
  EXPECT_EQ(test_map, *kIDPropertyPaddingMap);
}

TEST_F(CSSKeyframeManagerTest, PlatformMissingEndpointUsesIncomingBaseStyle) {
  manager->GetConfig()->SetEnableFiberArch(true);
  auto target = InitElement();
  ASSERT_TRUE(target->computed_css_style()->SetValue(
      kPropertyIDOpacity, CSSValue(0.4f, CSSValuePattern::NUMBER)));
  starlight::ComputedCSSStyle incoming(*target->computed_css_style());
  incoming.CopyFrom(*target->computed_css_style());
  ASSERT_TRUE(incoming.SetValue(kPropertyIDOpacity,
                                CSSValue(0.8f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(target.get(), base::String("test"), 0.0);
  auto animator = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data;
  data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  animator->SyncAnimationDataForNewPipeline(
      data, false, &incoming.GetResolvedValues(), nullptr,
      incoming.GetCustomProperties(), &incoming);
  ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
  const auto& frame = target->platform_animation_commands_->front()
                          .properties.front()
                          .keyframes.front();
  EXPECT_FLOAT_EQ(static_cast<const gfx::FloatKeyframe*>(frame.get())->Value(),
                  0.8f);

  target->platform_animation_commands_.reset();
  const StyleMap empty_base_styles;
  animator->SyncAnimationDataForNewPipeline(data, true, &empty_base_styles);
  ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
  const auto& default_frame = target->platform_animation_commands_->front()
                                  .properties.front()
                                  .keyframes.front();
  EXPECT_FLOAT_EQ(
      static_cast<const gfx::FloatKeyframe*>(default_frame.get())->Value(),
      1.0f);
}

TEST_F(CSSKeyframeManagerTest,
       ImplicitEndpointsUseSameUnderlyingValueAcrossExecutionPaths) {
  // Exercise New Animator with routing disabled, platform execution, and
  // fallback after platform endpoint resolution has succeeded.
  for (int route = 0; route < 3; ++route) {
    SCOPED_TRACE(route);
    manager->GetConfig()->SetEnableFiberArch(true);
    manager->supports_platform_animation_routing_ = route != 0;
    auto capabilities = gfx::GetIOSAnimationBackendCapabilities();
    if (route == 2) {
      capabilities.properties.clear();
    }
    routing_painting_context_->SetPlatformAnimationCapabilities(
        std::move(capabilities));
    auto target = InitElement();
    target->has_painting_node_ = true;
    ASSERT_TRUE(target->computed_css_style()->SetValue(
        kPropertyIDOpacity, CSSValue(0.2f, CSSValuePattern::NUMBER)));
    auto keyframes = lepus::Dictionary::Create();
    auto middle = lepus::Dictionary::Create();
    middle->SetValue("opacity", lepus::Value(0.5));
    keyframes->SetValue("50", lepus::Value(middle));
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *target->keyframes_map_, base::String("test"), lepus::Value(keyframes),
        target->element_manager()->GetCSSParserConfigs());
    auto animator = InitTestKeyframeManager(target.get());
    auto data = InitAnimationData(base::String("test"), 1000, 0,
                                  starlight::TimingFunctionData(), 1,
                                  starlight::AnimationFillModeType::kBoth,
                                  starlight::AnimationDirectionType::kNormal,
                                  starlight::AnimationPlayStateType::kRunning);

    // An incoming value overrides the committed value; an absent incoming
    // declaration uses the default instead of the committed value.
    for (bool has_incoming_value : {true, false}) {
      SCOPED_TRACE(has_incoming_value);
      StyleMap incoming;
      if (has_incoming_value) {
        incoming.emplace(kPropertyIDOpacity,
                         CSSValue(0.8f, CSSValuePattern::NUMBER));
      }
      const float expected = has_incoming_value ? 0.8f : 1.0f;
      auto result = animator->BuildAnimation(data, nullptr, &incoming);
      if (route == 1) {
        ASSERT_TRUE(result.platform_animation.has_value());
        EXPECT_EQ(result.platform_animation->underlying_endpoint_values.at(
                      kPropertyIDOpacity),
                  CSSValue(expected, CSSValuePattern::NUMBER));
        ASSERT_EQ(result.platform_animation->properties.size(), 1u);
        const auto& frames =
            result.platform_animation->properties.front().keyframes;
        ASSERT_EQ(frames.size(), 3u);
        for (const auto* endpoint :
             {frames.front().get(), frames.back().get()}) {
          EXPECT_FALSE(endpoint->IsEmpty());
          EXPECT_FLOAT_EQ(
              static_cast<const gfx::FloatKeyframe*>(endpoint)->Value(),
              expected);
          EXPECT_EQ(endpoint->timing_source(),
                    gfx::Keyframe::TimingSource::kAnimation);
        }
        EXPECT_FLOAT_EQ(
            static_cast<const gfx::FloatKeyframe*>(frames[1].get())->Value(),
            0.5f);
      } else {
        EXPECT_FALSE(result.platform_animation.has_value());
        ASSERT_NE(result.new_animator_animation, nullptr);
        auto* curve = result.new_animator_animation->keyframe_effect()
                          ->GetKeyframeModelByCurveType(
                              animation::AnimationCurve::CurveType::OPACITY)
                          ->animation_curve();
        ASSERT_EQ(curve->keyframes_.size(), 3u);
        auto* start = curve->keyframes_.front().get();
        auto* end = curve->keyframes_.back().get();
        auto* start_timing = start->timing_function();
        auto* end_timing = end->timing_function();
        curve->EnsureFromAndToKeyframe();
        ASSERT_EQ(curve->keyframes_.size(), 3u);
        EXPECT_EQ(curve->keyframes_.front().get(), start);
        EXPECT_EQ(curve->keyframes_.back().get(), end);
        EXPECT_TRUE(start->IsEmpty());
        EXPECT_TRUE(end->IsEmpty());
        for (double offset : {0.25, 0.75}) {
          auto time = fml::TimeDelta::FromSecondsF(offset);
          EXPECT_NEAR(curve->GetValue(time).AsNumber(), (expected + 0.5f) / 2,
                      1e-6);
        }
        result.new_animator_animation->UpdateUnderlyingValue(
            animation::AnimationCurve::CurveType::OPACITY,
            CSSValue(0.4f, CSSValuePattern::NUMBER));
        EXPECT_EQ(curve->keyframes_.front().get(), start);
        EXPECT_EQ(curve->keyframes_.back().get(), end);
        EXPECT_TRUE(start->IsEmpty());
        EXPECT_TRUE(end->IsEmpty());
        EXPECT_EQ(start->timing_function(), start_timing);
        EXPECT_EQ(end->timing_function(), end_timing);
        EXPECT_FLOAT_EQ(
            static_cast<const gfx::FloatKeyframe*>(curve->keyframes_[1].get())
                ->Value(),
            0.5f);
        for (double offset : {0.25, 0.75}) {
          auto time = fml::TimeDelta::FromSecondsF(offset);
          EXPECT_NEAR(curve->GetValue(time).AsNumber(), 0.45, 1e-6);
        }
        // Reset and subsequent updates change the sampling input, without
        // creating or mutating endpoint frames.
        result.new_animator_animation->UpdateUnderlyingValue(
            animation::AnimationCurve::CurveType::OPACITY, CSSValue());
        EXPECT_TRUE(start->IsEmpty());
        EXPECT_TRUE(end->IsEmpty());
        auto time = fml::TimeDelta::FromSecondsF(0.25);
        EXPECT_NEAR(curve->GetValue(time).AsNumber(), 0.75, 1e-6);
        result.new_animator_animation->UpdateUnderlyingValue(
            animation::AnimationCurve::CurveType::OPACITY,
            CSSValue(0.6f, CSSValuePattern::NUMBER));
        EXPECT_EQ(curve->keyframes_.front().get(), start);
        EXPECT_TRUE(start->IsEmpty());
        EXPECT_TRUE(end->IsEmpty());
        time = fml::TimeDelta::FromSecondsF(0.25);
        EXPECT_NEAR(curve->GetValue(time).AsNumber(), 0.55, 1e-6);
      }
    }
  }
}

TEST_F(CSSKeyframeManagerTest, NeutralEndpointResolvesCalcAfterParentResize) {
  manager->supports_platform_animation_routing_ = false;
  auto parent = InitElement();
  auto target = InitElement();
  target->set_parent(parent.get());
  parent->width_ = 200.f;
  UpdateToOnlyLeftKeyframes(target.get(), base::String("test"), "0px");
  auto animator = InitTestKeyframeManager(target.get());
  auto data = InitAnimationData(base::String("test"), 1000, 0,
                                starlight::TimingFunctionData(), 1,
                                starlight::AnimationFillModeType::kBoth,
                                starlight::AnimationDirectionType::kNormal,
                                starlight::AnimationPlayStateType::kRunning);
  StyleMap underlying;
  underlying.emplace(kPropertyIDLeft,
                     CSSValue("calc(50% + 10px)", CSSValuePattern::CALC,
                              CSSValueType::DEFAULT));
  auto result = animator->BuildAnimation(data, nullptr, &underlying);
  ASSERT_NE(result.new_animator_animation, nullptr);
  auto* curve = result.new_animator_animation->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::LEFT)
                    ->animation_curve();
  ASSERT_EQ(curve->keyframes_.size(), 2u);
  auto* start = curve->keyframes_.front().get();
  ASSERT_TRUE(start->IsEmpty());
  auto time = fml::TimeDelta::FromSecondsF(0.5);
  EXPECT_NEAR(curve->GetValue(time).AsNumber(), 55.f, 1e-6);

  // The expression is unchanged; resolve it against the new parent size
  // without rebuilding keyframes or pushing a new underlying CSS value.
  parent->width_ = 400.f;
  time = fml::TimeDelta::FromSecondsF(0.5);
  EXPECT_NEAR(curve->GetValue(time).AsNumber(), 105.f, 1e-6);
  EXPECT_EQ(curve->keyframes_.front().get(), start);
  EXPECT_TRUE(start->IsEmpty());
}

TEST_F(CSSKeyframeManagerTest,
       PlatformCommandsFollowNodeUpdatesWithoutPropBundle) {
  manager->GetConfig()->SetEnableFiberArch(true);
  auto target = InitElement();
  target->MarkCanBeLayoutOnly(false);
  int submissions = 0;
  int expected_property = 0;
  std::shared_ptr<gfx::PlatformAnimationCommandBatch> expected_batch;
  routing_painting_context_->on_animation_commands = [&](int id,
                                                         const auto& commands) {
    ++submissions;
    EXPECT_EQ(id, target->impl_id());
    EXPECT_EQ(commands, expected_batch);
    const auto& nodes = routing_painting_context_->node_map_;
    auto node = nodes.find(id);
    ASSERT_NE(node, nodes.end());
    const auto& props = node->second->props_;
    auto property = props.find("test-property");
    ASSERT_NE(property, props.end());
    EXPECT_EQ(property->second.Number(), expected_property);
  };

  const gfx::PlatformAnimationCommandType types[] = {
      gfx::PlatformAnimationCommandType::kHandoff,
      gfx::PlatformAnimationCommandType::kUpdate,
      gfx::PlatformAnimationCommandType::kCancel};
  for (int i = 0; i < 3; ++i) {
    SCOPED_TRACE(i);
    if (i < 2) {
      expected_property = i;
      target->PreparePropBundleIfNeed();
      target->prop_bundle_->SetProps("test-property", i);
    }
    gfx::PlatformAnimationCommand command;
    command.type = types[i];
    target->QueuePlatformAnimationCommand(std::move(command));
    EXPECT_NE(target->dirty_ & Element::kDirtyForceUpdate, 0u);
    expected_batch = target->platform_animation_commands_;
    if (i == 2) {
      // A command-only update must neither allocate nor depend on a bundle.
      EXPECT_EQ(target->prop_bundle_, nullptr);
      target->ResetPropBundle();
      EXPECT_TRUE(target->HasPendingPlatformAnimationCommands());
      target->MarkParallelFlushFlag(Element::kFlagGreedyParallel);
    }
    target->PerformElementContainerCreateOrUpdate(false, true);
    EXPECT_EQ(target->dirty_ & Element::kDirtyForceUpdate, 0u);
    if (i == 2) {
      EXPECT_EQ(submissions, i);
      // Parallel commits stay deferred until the normal reduction phase.
      auto finish_parallel_update = target->CreateParallelTaskHandler();
      finish_parallel_update();
    }
    EXPECT_EQ(submissions, i + 1);
    EXPECT_FALSE(target->HasPendingPlatformAnimationCommands());
  }
  target->PerformElementContainerCreateOrUpdate(false, true);
  EXPECT_EQ(submissions, 3);
  routing_painting_context_->on_animation_commands = nullptr;
}

TEST_F(CSSKeyframeManagerTest, PlatformHandoffsPreserveAnimationListOrder) {
  manager->GetConfig()->SetEnableFiberArch(true);
  auto target = InitElement();
  auto animator = InitTestKeyframeManager(target.get());
  base::Vector<starlight::AnimationData> data;
  for (const auto* name : {"fade_a", "fade_b", "fade_c"}) {
    UpdateOpacityKeyframes(target.get(), base::String(name), 0.0, 1.0);
    data.emplace_back(InitAnimationData(
        base::String(name), 1000, 0, starlight::TimingFunctionData(), 1,
        starlight::AnimationFillModeType::kBoth,
        starlight::AnimationDirectionType::kNormal,
        starlight::AnimationPlayStateType::kRunning));
  }
  animator->SyncAnimationDataForNewPipeline(data);
  ASSERT_TRUE(target->HasPendingPlatformAnimationCommands());
  ASSERT_EQ(target->platform_animation_commands_->size(), 3u);
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ((*target->platform_animation_commands_)[i].name,
              data[i].name.str());
  }
  target->platform_animation_commands_.reset();
  std::swap(data[0], data[2]);
  animator->SyncAnimationDataForNewPipeline(data);
  ASSERT_EQ(target->platform_animation_commands_->size(), 3u);
  for (size_t i = 0; i < data.size(); ++i) {
    const auto& command = (*target->platform_animation_commands_)[i];
    EXPECT_EQ(command.name, data[i].name.str());
    EXPECT_EQ(command.type, gfx::PlatformAnimationCommandType::kUpdate);
    EXPECT_EQ(command.generation, 2u);
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

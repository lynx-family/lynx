// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/inspector_animation_controller.h"

#include <chrono>
#include <future>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/animation/animation.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace testing {
namespace {

constexpr int32_t kWidth = 1080;
constexpr int32_t kHeight = 1920;
constexpr float kDefaultLayoutsUnitPerPx = 1.f;
constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class RecordingMessageSender : public devtool::MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    json_messages_.emplace_back(type, msg);
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    string_messages_.emplace_back(type, msg);
  }

  std::vector<std::pair<std::string, Json::Value>> json_messages_;
  std::vector<std::pair<std::string, std::string>> string_messages_;
};

std::shared_ptr<devtool::CDPResponder> MakeResponder(
    const std::shared_ptr<devtool::MessageSender>& sender,
    const Json::Value& message) {
  return std::make_shared<devtool::CDPResponder>(sender,
                                                 message["id"].asInt64());
}

class InspectorAnimationControllerTest : public ::testing::Test {
 public:
  void SetUp() override {
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    const lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<lynx::tasm::MockPaintingContext>(),
        tasm_mediator_.get(), lynx_env_config);

    devtool_mediator_ = std::make_shared<lynx::devtool::LynxDevToolMediator>();
    devtools_ng_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    message_sender_ = std::make_shared<devtool::MessageSenderMock>();
    devtools_ng_->message_sender_ = message_sender_;
    devtool_mediator_->devtool_wp_ = devtools_ng_;
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    devtool_mediator_->default_task_runner_ = ui_thread_->GetTaskRunner();
    controller_ = std::make_unique<devtool::InspectorAnimationController>(
        devtool_mediator_);
  }

  std::shared_ptr<animation::Animation> BuildAnimationForSnapshot(
      const std::string& name, int64_t duration_ms,
      std::initializer_list<double> normalized_offsets,
      animation::Animation::Origin origin,
      starlight::TimingFunctionType animation_timing =
          starlight::TimingFunctionType::kLinear,
      const std::map<double, std::string>& keyframe_easing_values = {}) {
    auto animation = std::make_shared<animation::Animation>(name);
    auto element = manager_->CreateFiberElement("view");
    animation->BindElement(element.get());

    auto effect = animation::KeyframeEffect::Create();
    auto curve = animation::KeyframedOpacityAnimationCurve::Create();
    auto authored_keyframes = lepus::Dictionary::Create();
    for (double offset : normalized_offsets) {
      auto frame_style = lepus::Dictionary::Create();
      frame_style->SetValue("opacity", lepus::Value(offset));
      // CSSKeyframeManager always creates a timing function, including an
      // implicit Linear object when the author did not specify easing.
      std::unique_ptr<gfx::TimingFunction> timing_function =
          gfx::LinearTimingFunction::Create();
      const auto easing = keyframe_easing_values.find(offset);
      if (easing != keyframe_easing_values.end()) {
        frame_style->SetValue("animation-timing-function",
                              lepus::Value(easing->second));
        if (easing->second == "linear") {
          timing_function = gfx::LinearTimingFunction::Create();
        } else if (easing->second == "ease-out") {
          timing_function = gfx::CubicBezierTimingFunction::CreatePreset(
              gfx::CubicBezierTimingFunction::EaseType::EASE_OUT);
        }
      }
      authored_keyframes->SetValue(std::to_string(offset * 100),
                                   lepus::Value(frame_style));
      auto keyframe = gfx::FloatKeyframe::Create(
          fml::TimeDelta::FromSecondsF(static_cast<float>(offset)),
          std::move(timing_function));
      keyframe->SetValue(static_cast<float>(offset));
      curve->AddKeyframe(std::move(keyframe));
    }
    curve->type_ = animation::AnimationCurve::CurveType::OPACITY;
    effect->AddKeyframeModel(
        animation::KeyframeModel::Create(std::move(curve)));
    animation->SetKeyframeEffect(std::move(effect));

    starlight::AnimationData data;
    data.name = base::String(name);
    data.duration = duration_ms;
    data.timing_func.timing_func = animation_timing;
    animation->UpdateAnimationData(data);
    animation->SetOrigin(origin);
    if (origin == animation::Animation::Origin::kCSSAnimation) {
      starlight::CSSStyleUtils::UpdateCSSKeyframes(
          *element->keyframes_map_, data.name, lepus::Value(authored_keyframes),
          tasm::CSSParserConfigs());
    }

    snapshot_elements_.push_back(std::move(element));
    return animation;
  }

  Json::Value BuildAnimationSnapshot(animation::Animation* animation) {
    return controller_->BuildAnimationSnapshot(animation);
  }

  void FlushDevtoolTasks() {
    std::promise<void> p;
    auto f = p.get_future();
    ASSERT_TRUE(
        devtool_mediator_->RunOnDevToolThread([&p]() { p.set_value(); }, true));
    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
  }

 protected:
  std::unique_ptr<devtool::InspectorAnimationController> controller_;
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<lynx::testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<lynx::tasm::ElementManager> manager_;
  std::vector<fml::RefPtr<tasm::Element>> snapshot_elements_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator_;
};

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotFormatsCSSKeyframeOffsetsAsPercentages) {
  auto animation =
      BuildAnimationForSnapshot("orbit", 2400, {0.0, 0.25, 1.0},
                                animation::Animation::Origin::kCSSAnimation);

  const Json::Value snapshot = BuildAnimationSnapshot(animation.get());
  const Json::Value& keyframes =
      snapshot["source"]["keyframesRule"]["keyframes"];

  ASSERT_EQ(keyframes.size(), 3U);
  EXPECT_EQ(keyframes[0]["offset"].asString(), "0%");
  EXPECT_EQ(keyframes[1]["offset"].asString(), "25%");
  EXPECT_EQ(keyframes[2]["offset"].asString(), "100%");
  EXPECT_EQ(snapshot["type"].asString(), "CSSAnimation");
  EXPECT_GT(snapshot["source"]["backendNodeId"].asInt(), 0);
}

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotSeparatesEffectAndCSSKeyframeTiming) {
  auto animation =
      BuildAnimationForSnapshot("orbit", 2400, {0.0, 0.45, 1.0},
                                animation::Animation::Origin::kCSSAnimation,
                                starlight::TimingFunctionType::kEaseInEaseOut);

  const Json::Value snapshot = BuildAnimationSnapshot(animation.get());
  const Json::Value& source = snapshot["source"];
  const Json::Value& keyframes = source["keyframesRule"]["keyframes"];

  EXPECT_EQ(source["easing"].asString(), "ease-in-out");
  ASSERT_EQ(keyframes.size(), 3U);
  EXPECT_EQ(keyframes[0]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[1]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[2]["easing"].asString(), "linear");
}

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotPreservesExplicitCSSKeyframeTiming) {
  auto animation =
      BuildAnimationForSnapshot("orbit", 2400, {0.0, 0.45, 1.0},
                                animation::Animation::Origin::kCSSAnimation,
                                starlight::TimingFunctionType::kEaseInEaseOut,
                                {{0.45, "linear"}, {1.0, "ease-out"}});

  const Json::Value snapshot = BuildAnimationSnapshot(animation.get());
  const Json::Value& keyframes =
      snapshot["source"]["keyframesRule"]["keyframes"];

  ASSERT_EQ(keyframes.size(), 3U);
  EXPECT_EQ(snapshot["source"]["easing"].asString(), "ease-in-out");
  EXPECT_EQ(keyframes[0]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[1]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[2]["easing"].asString(), "ease-out");
}

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotIgnoresChangedAndRemovedSourceRules) {
  auto animation =
      BuildAnimationForSnapshot("orbit", 2400, {0.0, 0.45, 1.0},
                                animation::Animation::Origin::kCSSAnimation,
                                starlight::TimingFunctionType::kEaseInEaseOut,
                                {{0.45, "linear"}, {1.0, "ease-out"}});
  auto& rules = *animation->GetElement()->keyframes_map_;
  const auto original = BuildAnimationSnapshot(animation.get());
  auto& styles = rules[base::String("orbit")]->GetKeyframesContent();
  // The model still has ease-out at 100%, even if its source no longer does.
  styles[1.0f]->erase(tasm::kPropertyIDAnimationTimingFunction);
  rules[base::String("orbit")]->GetKeyframesContent().erase(0.0f);
  auto snapshot = BuildAnimationSnapshot(animation.get());
  EXPECT_EQ(snapshot["source"], original["source"]);

  // Rule cleanup must not alter metadata for an already-created model.
  rules.clear();
  snapshot = BuildAnimationSnapshot(animation.get());
  EXPECT_EQ(snapshot["source"], original["source"]);
  const auto& keyframes = snapshot["source"]["keyframesRule"]["keyframes"];
  ASSERT_EQ(keyframes.size(), 3U);
  EXPECT_EQ(keyframes[0]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[1]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[2]["easing"].asString(), "ease-out");
}

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotKeepsWebAnimationTimingOnSource) {
  auto animation = BuildAnimationForSnapshot(
      "_lynx-inner-js-animation-0", 1800, {0.0, 0.5, 1.0},
      animation::Animation::Origin::kWebAnimation);
  starlight::AnimationData data = animation->get_animation_data();
  data.timing_func.timing_func = starlight::TimingFunctionType::kCubicBezier;
  data.timing_func.x1 = 0.65f;
  data.timing_func.y1 = 0.0f;
  data.timing_func.x2 = 0.35f;
  data.timing_func.y2 = 1.0f;
  animation->UpdateAnimationData(data);

  const Json::Value snapshot = BuildAnimationSnapshot(animation.get());
  const Json::Value& source = snapshot["source"];
  const Json::Value& keyframes = source["keyframesRule"]["keyframes"];

  EXPECT_EQ(snapshot["type"].asString(), "WebAnimation");
  EXPECT_EQ(source["easing"].asString(), "cubic-bezier(0.65, 0, 0.35, 1)");
  ASSERT_EQ(keyframes.size(), 3U);
  EXPECT_EQ(keyframes[0]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[1]["easing"].asString(), "linear");
  EXPECT_EQ(keyframes[2]["easing"].asString(), "linear");
}

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotFormatsTransitionOffsetsAsPercentages) {
  auto animation =
      BuildAnimationForSnapshot("transform", 550, {0.0, 1.0},
                                animation::Animation::Origin::kCSSTransition);

  const Json::Value snapshot = BuildAnimationSnapshot(animation.get());
  const Json::Value& keyframes =
      snapshot["source"]["keyframesRule"]["keyframes"];

  ASSERT_EQ(keyframes.size(), 2U);
  EXPECT_EQ(keyframes[0]["offset"].asString(), "0%");
  EXPECT_EQ(keyframes[1]["offset"].asString(), "100%");
  EXPECT_EQ(snapshot["type"].asString(), "CSSTransition");
}

TEST_F(InspectorAnimationControllerTest,
       BuildAnimationSnapshotOmitsIterationsForInfiniteAnimation) {
  auto animation = BuildAnimationForSnapshot(
      "orbit", 2400, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  auto data = animation->get_animation_data();
  data.iteration_count = 1000000000;  // CSS parser's `infinite` sentinel.
  animation->UpdateAnimationData(data);

  const Json::Value snapshot = BuildAnimationSnapshot(animation.get());
  EXPECT_FALSE(snapshot["source"].isMember("iterations"));
}

TEST_F(InspectorAnimationControllerTest,
       EnableBackfillsCreatedBeforeStartedEvents) {
  auto first = BuildAnimationForSnapshot(
      "orbit", 2400, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  auto second =
      BuildAnimationForSnapshot("corePulse", 1200, {0.0, 1.0},
                                animation::Animation::Origin::kCSSAnimation);
  controller_->OnAnimationCreated(first.get());
  controller_->OnAnimationCreated(second.get());

  auto event_sender = std::make_shared<RecordingMessageSender>();
  devtools_ng_->message_sender_ = event_sender;
  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 201;

  controller_->Enable(MakeResponder(response_sender, message),
                      message["params"]);
  FlushDevtoolTasks();

  ASSERT_EQ(event_sender->json_messages_.size(), 4U);
  EXPECT_EQ(event_sender->json_messages_[0].second["method"].asString(),
            "Animation.animationCreated");
  EXPECT_EQ(event_sender->json_messages_[1].second["method"].asString(),
            "Animation.animationCreated");
  EXPECT_EQ(event_sender->json_messages_[2].second["method"].asString(),
            "Animation.animationStarted");
  EXPECT_EQ(event_sender->json_messages_[3].second["method"].asString(),
            "Animation.animationStarted");

  const std::string first_created =
      event_sender->json_messages_[0].second["params"]["id"].asString();
  const std::string second_created =
      event_sender->json_messages_[1].second["params"]["id"].asString();
  const std::string first_started = event_sender->json_messages_[2]
                                        .second["params"]["animation"]["id"]
                                        .asString();
  const std::string second_started = event_sender->json_messages_[3]
                                         .second["params"]["animation"]["id"]
                                         .asString();
  EXPECT_EQ(first_started, first_created);
  EXPECT_EQ(second_started, second_created);
}

TEST_F(InspectorAnimationControllerTest,
       SeekAnimationsSeeksBatchWithSharedReferenceTime) {
  auto first = BuildAnimationForSnapshot(
      "orbit", 3000, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  auto second = BuildAnimationForSnapshot(
      "pulse", 3000, {0.0, 1.0}, animation::Animation::Origin::kWebAnimation);
  controller_->OnAnimationCreated(first.get());
  controller_->OnAnimationCreated(second.get());

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 202;
  message["params"]["animations"] = Json::Value(Json::arrayValue);
  message["params"]["animations"].append(std::to_string(first->id()));
  message["params"]["animations"].append(std::to_string(second->id()));
  message["params"]["animations"].append(std::to_string(first->id()));
  message["params"]["currentTime"] = 1250.5;

  controller_->SeekAnimations(MakeResponder(response_sender, message),
                              message["params"]);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_TRUE(response.isMember("result"));
  EXPECT_FALSE(response.isMember("error"));
  EXPECT_EQ(first->GetState(), animation::Animation::State::kPlay);
  EXPECT_EQ(second->GetState(), animation::Animation::State::kPlay);
  EXPECT_EQ(first->start_time(), second->start_time());
}

TEST_F(InspectorAnimationControllerTest,
       SeekAnimationsRejectsInvalidBatchAtomically) {
  auto animation = BuildAnimationForSnapshot(
      "orbit", 3000, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  controller_->OnAnimationCreated(animation.get());
  const auto original_start_time = animation->start_time();
  const auto original_state = animation->GetState();

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 203;
  message["params"]["animations"] = Json::Value(Json::arrayValue);
  message["params"]["animations"].append(std::to_string(animation->id()));
  message["params"]["animations"].append("999999999999");
  message["params"]["currentTime"] = 500;

  controller_->SeekAnimations(MakeResponder(response_sender, message),
                              message["params"]);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_EQ(response["error"]["code"].asInt(), -32602);
  EXPECT_EQ(animation->start_time(), original_start_time);
  EXPECT_EQ(animation->GetState(), original_state);
}

TEST_F(InspectorAnimationControllerTest,
       SeekAnimationsValidatesTimeAndAllowsEmptyBatch) {
  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 204;
  message["params"]["animations"] = Json::Value(Json::arrayValue);
  message["params"]["currentTime"] = -1;

  controller_->SeekAnimations(MakeResponder(response_sender, message),
                              message["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            -32602);

  response_sender->json_messages_.clear();
  message["id"] = 205;
  message["params"]["currentTime"] = std::numeric_limits<double>::quiet_NaN();
  controller_->SeekAnimations(MakeResponder(response_sender, message),
                              message["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            -32602);

  response_sender->json_messages_.clear();
  message["id"] = 206;
  message["params"]["currentTime"] = 0;
  controller_->SeekAnimations(MakeResponder(response_sender, message),
                              message["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_TRUE(response_sender->json_messages_[0].second.isMember("result"));
}

TEST_F(InspectorAnimationControllerTest,
       SetPausedPausesAndResumesBatchAtSharedReferenceTime) {
  auto first = BuildAnimationForSnapshot(
      "orbit", 3000, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  auto second = BuildAnimationForSnapshot(
      "pulse", 3000, {0.0, 1.0}, animation::Animation::Origin::kWebAnimation);
  first->Play(false);
  second->Play(false);
  auto start_time = fml::TimePoint::Now();
  first->SampleAt(start_time);
  second->SampleAt(start_time);
  // Give both Inspector clocks the same origin before testing batch controls.
  first->SeekTo(fml::TimeDelta::Zero(), start_time);
  second->SeekTo(fml::TimeDelta::Zero(), start_time);
  controller_->OnAnimationCreated(first.get());
  controller_->OnAnimationCreated(second.get());

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 207;
  message["params"]["animations"] = Json::Value(Json::arrayValue);
  message["params"]["animations"].append(std::to_string(first->id()));
  message["params"]["animations"].append(std::to_string(second->id()));
  message["params"]["animations"].append(std::to_string(first->id()));
  message["params"]["paused"] = true;

  controller_->SetPaused(MakeResponder(response_sender, message),
                         message["params"]);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_TRUE(response_sender->json_messages_[0].second.isMember("result"));
  EXPECT_EQ(first->GetState(), animation::Animation::State::kPause);
  EXPECT_EQ(second->GetState(), animation::Animation::State::kPause);
  EXPECT_EQ(first->pause_time(), second->pause_time());
  EXPECT_EQ(first->GetCurrentTime(), second->GetCurrentTime());

  response_sender->json_messages_.clear();
  message["id"] = 208;
  message["params"]["paused"] = false;
  controller_->SetPaused(MakeResponder(response_sender, message),
                         message["params"]);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_TRUE(response_sender->json_messages_[0].second.isMember("result"));
  EXPECT_EQ(first->GetState(), animation::Animation::State::kPlay);
  EXPECT_EQ(second->GetState(), animation::Animation::State::kPlay);
  EXPECT_EQ(first->total_paused_duration(), second->total_paused_duration());
}

TEST_F(InspectorAnimationControllerTest,
       SetPausedRejectsInvalidBatchAtomically) {
  auto animation = BuildAnimationForSnapshot(
      "orbit", 3000, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  animation->Play(false);
  auto start_time = fml::TimePoint::Now();
  animation->SampleAt(start_time);
  controller_->OnAnimationCreated(animation.get());

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 209;
  message["params"]["animations"] = Json::Value(Json::arrayValue);
  message["params"]["animations"].append(std::to_string(animation->id()));
  message["params"]["animations"].append("999999999999");
  message["params"]["paused"] = true;

  controller_->SetPaused(MakeResponder(response_sender, message),
                         message["params"]);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            -32602);
  EXPECT_EQ(animation->GetState(), animation::Animation::State::kPlay);
  EXPECT_EQ(animation->pause_time(), fml::TimePoint::Min());
}

TEST_F(InspectorAnimationControllerTest,
       SetPausedValidatesBooleanAndAllowsEmptyBatch) {
  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::objectValue);
  message["id"] = 210;
  message["params"]["animations"] = Json::Value(Json::arrayValue);
  message["params"]["paused"] = 1;

  controller_->SetPaused(MakeResponder(response_sender, message),
                         message["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            -32602);

  response_sender->json_messages_.clear();
  message["id"] = 211;
  message["params"]["paused"] = false;
  controller_->SetPaused(MakeResponder(response_sender, message),
                         message["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_TRUE(response_sender->json_messages_[0].second.isMember("result"));
}

TEST_F(InspectorAnimationControllerTest,
       ReleaseAnimationsRejectsInvalidBatchAtomically) {
  auto animation = BuildAnimationForSnapshot(
      "orbit", 3000, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  controller_->OnAnimationCreated(animation.get());

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value release(Json::objectValue);
  release["id"] = 212;
  release["params"]["animations"] = Json::Value(Json::arrayValue);
  release["params"]["animations"].append(std::to_string(animation->id()));
  release["params"]["animations"].append(42);

  controller_->ReleaseAnimations(MakeResponder(response_sender, release),
                                 release["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            -32602);

  response_sender->json_messages_.clear();
  Json::Value get_current_time(Json::objectValue);
  get_current_time["id"] = 213;
  get_current_time["params"]["id"] = std::to_string(animation->id());
  controller_->GetCurrentTime(MakeResponder(response_sender, get_current_time),
                              get_current_time["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_TRUE(response_sender->json_messages_[0].second.isMember("result"));
}

TEST_F(InspectorAnimationControllerTest,
       ReleaseAnimationsSurvivesLateStartedCallback) {
  auto animation = BuildAnimationForSnapshot(
      "orbit", 3000, {0.0, 1.0}, animation::Animation::Origin::kCSSAnimation);
  controller_->OnAnimationCreated(animation.get());

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value release(Json::objectValue);
  release["id"] = 214;
  release["params"]["animations"] = Json::Value(Json::arrayValue);
  release["params"]["animations"].append(std::to_string(animation->id()));
  controller_->ReleaseAnimations(MakeResponder(response_sender, release),
                                 release["params"]);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_TRUE(response_sender->json_messages_[0].second.isMember("result"));

  // Created may be missed while the observer is being installed. A later
  // started callback is normally the fallback registration point, but must not
  // resurrect an ID the frontend has already released.
  controller_->OnAnimationStarted(animation.get());

  response_sender->json_messages_.clear();
  Json::Value get_current_time(Json::objectValue);
  get_current_time["id"] = 215;
  get_current_time["params"]["id"] = std::to_string(animation->id());
  controller_->GetCurrentTime(MakeResponder(response_sender, get_current_time),
                              get_current_time["params"]);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            -32602);
}

}  // namespace
}  // namespace testing
}  // namespace lynx

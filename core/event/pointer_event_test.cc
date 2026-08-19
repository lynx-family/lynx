// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/pointer_event.h"

#include <array>
#include <memory>
#include <string>

#include "core/event/event_target_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace event {
namespace test {

TEST(PointerEventTest, UsesWebPropagationSemantics) {
  struct ExpectedSemantics {
    const char* name;
    bool bubbles;
    bool cancelable;
    bool composed;
  };
  constexpr std::array<ExpectedSemantics, 8> kExpected = {{
      {"pointerdown", true, true, true},
      {"pointermove", true, true, true},
      {"pointerup", true, true, true},
      {"pointercancel", true, false, true},
      {"pointerover", true, true, true},
      {"pointerenter", false, false, false},
      {"pointerout", true, true, true},
      {"pointerleave", false, false, false},
  }};

  for (const auto& expected : kExpected) {
    auto event = fml::MakeRefCounted<PointerEvent>(
        expected.name, lepus::Value(lepus::Dictionary::Create()), 123);
    EXPECT_EQ(event->event_type(), Event::EventType::kPointerEvent);
    EXPECT_TRUE(event->IsCaptureBubbleEvent());
    EXPECT_EQ(event->bubbles(), expected.bubbles);
    EXPECT_EQ(event->cancelable(), expected.cancelable);
    EXPECT_EQ(event->composed(), expected.composed);
    EXPECT_EQ(event->time_stamp(), 123);
  }
}

TEST(PointerEventTest, ResolvesRelatedTargetWithoutExposingInternalSign) {
  auto params = lepus::Dictionary::Create();
  params->SetValue(kPointerRelatedTargetSign, 42);
  auto event = fml::MakeRefCounted<PointerEvent>("pointerover",
                                                 lepus::Value(params), 123);

  EXPECT_EQ(event->related_target_sign(), 42);
  EXPECT_FALSE(event->detail().Table()->Contains(kPointerRelatedTargetSign));

  auto target = std::make_unique<MockEventTarget>();
  auto related_target = std::make_unique<MockEventTarget>();
  auto related_target_info = lepus::Dictionary::Create();
  related_target_info->SetValue("id", "related");
  related_target->SetEventTargetInfo(lepus::Value(related_target_info));
  event->set_target(target->GetWeakTarget());
  event->set_current_target(target->GetWeakTarget());
  event->set_related_target(related_target->GetWeakTarget());
  event->HandleEventBaseDetail();

  const auto& related = event->detail().Table()->GetValue("relatedTarget");
  ASSERT_TRUE(related.IsTable());
  EXPECT_EQ(related.Table()->GetValue("id").StdString(), "related");
}

TEST(PointerEventTest, UsesNullRelatedTargetWhenSignIsInvalid) {
  auto target = std::make_unique<MockEventTarget>();
  auto event = fml::MakeRefCounted<PointerEvent>(
      "pointerleave", lepus::Value(lepus::Dictionary::Create()), 123);
  event->set_target(target->GetWeakTarget());
  event->set_current_target(target->GetWeakTarget());
  event->HandleEventBaseDetail();

  EXPECT_EQ(event->related_target_sign(), kInvalidPointerRelatedTargetSign);
  EXPECT_TRUE(event->detail().Table()->GetValue("relatedTarget").IsNil());
}

TEST(PointerEventTest, NormalizesMissingWebFields) {
  auto params = lepus::Dictionary::Create();
  params->SetValue("x", 12);
  params->SetValue("y", 34);
  params->SetValue("buttons", 1);
  auto event = fml::MakeRefCounted<PointerEvent>("pointerdown",
                                                 lepus::Value(params), 123);
  const auto& detail = event->detail().Table();

  EXPECT_EQ(detail->GetValue("offsetX").Number(), 12);
  EXPECT_EQ(detail->GetValue("offsetY").Number(), 34);
  EXPECT_EQ(detail->GetValue("width").Number(), 1);
  EXPECT_EQ(detail->GetValue("height").Number(), 1);
  EXPECT_EQ(detail->GetValue("pressure").Number(), 0.5);
  EXPECT_EQ(detail->GetValue("tangentialPressure").Number(), 0);
  EXPECT_EQ(detail->GetValue("tiltX").Number(), 0);
  EXPECT_EQ(detail->GetValue("tiltY").Number(), 0);
  EXPECT_EQ(detail->GetValue("twist").Number(), 0);
}

TEST(PointerEventTest, PreservesPlatformGeometryAndPressure) {
  auto params = lepus::Dictionary::Create();
  params->SetValue("width", 8);
  params->SetValue("height", 6);
  params->SetValue("pressure", 0.75);
  auto event = fml::MakeRefCounted<PointerEvent>("pointermove",
                                                 lepus::Value(params), 123);
  const auto& detail = event->detail().Table();

  EXPECT_EQ(detail->GetValue("width").Number(), 8);
  EXPECT_EQ(detail->GetValue("height").Number(), 6);
  EXPECT_EQ(detail->GetValue("pressure").Number(), 0.75);
}

TEST(PointerEventTest, NormalizesZeroTimestamp) {
  auto params = lepus::Dictionary::Create();
  params->SetValue("timestamp", 0);
  auto event =
      fml::MakeRefCounted<PointerEvent>("pointermove", lepus::Value(params), 0);

  EXPECT_GT(event->time_stamp(), 0);
  EXPECT_EQ(event->detail().Table()->GetValue("timestamp").Number(),
            event->time_stamp());
}

}  // namespace test
}  // namespace event
}  // namespace lynx

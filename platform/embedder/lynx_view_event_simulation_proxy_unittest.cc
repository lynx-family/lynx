// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_event_simulation_proxy.h"

#include <string>
#include <utility>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

struct SyntheticPointerEventRecord {
  enum class Source { kMouse, kTouch } source;
  std::string type;
  int x;
  int y;
  std::string button;
  float delta_x;
  float delta_y;
  int modifiers;
  int click_count;
};

class FakeEventSimulationTarget : public LynxViewEventSimulationTarget {
 public:
  void EmulateMouseSyntheticEvent(const std::string& event_type, int x, int y,
                                  const std::string& button, float delta_x,
                                  float delta_y, int modifiers,
                                  int click_count) override {
    events.push_back({SyntheticPointerEventRecord::Source::kMouse, event_type,
                      x, y, button, delta_x, delta_y, modifiers, click_count});
  }
  void EmulateTouchSyntheticEvent(const std::string& event_type, int x, int y,
                                  const std::string& button, float delta_x,
                                  float delta_y, int modifiers,
                                  int click_count) override {
    events.push_back({SyntheticPointerEventRecord::Source::kTouch, event_type,
                      x, y, button, delta_x, delta_y, modifiers, click_count});
  }

  void Focus(int node_id) override { focused_node_ids.push_back(node_id); }

  void InsertText(const std::string& text) override {
    inserted_texts.push_back(text);
  }

  std::vector<SyntheticPointerEventRecord> events;
  std::vector<int> focused_node_ids;
  std::vector<std::string> inserted_texts;
};

class LynxViewEventSimulationProxyTest : public testing::Test {
 protected:
  LynxViewEventSimulationProxy CreateProxy(
      FakeEventSimulationTarget** out_target) {
    auto target = std::make_unique<FakeEventSimulationTarget>();
    *out_target = target.get();
    return LynxViewEventSimulationProxy(std::move(target));
  }
};

TEST_F(LynxViewEventSimulationProxyTest, ForwardsFocusAndInsertText) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.Focus(18);
  proxy.InsertText("Lynx DevTool input");

  EXPECT_EQ(target->focused_node_ids, std::vector<int>({18}));
  EXPECT_EQ(target->inserted_texts,
            std::vector<std::string>({"Lynx DevTool input"}));
}

TEST_F(LynxViewEventSimulationProxyTest, EmulateTouchForwardsUnchanged) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mousePressed", 100, 200, "left", 1.5f, -2.5f, 4, 1);

  ASSERT_EQ(target->events.size(), 1u);
  EXPECT_EQ(target->events[0].source,
            SyntheticPointerEventRecord::Source::kTouch);
  EXPECT_EQ(target->events[0].type, "mousePressed");
  EXPECT_EQ(target->events[0].x, 100);
  EXPECT_EQ(target->events[0].button, "left");
  EXPECT_EQ(target->events[0].modifiers, 4);
  EXPECT_EQ(target->events[0].click_count, 1);
  EXPECT_FLOAT_EQ(target->events[0].delta_x, 1.5f);
  EXPECT_FLOAT_EQ(target->events[0].delta_y, -2.5f);
}

TEST_F(LynxViewEventSimulationProxyTest, EmulateTouchWheelDowngradesToMouse) {
  // Clay has no touch-wheel; the proxy converts to a mouse wheel with flipped
  // deltas to match Clay's sign convention.
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mouseWheel", 7, 8, "none", 1.5f, -2.5f, 0, 0);

  ASSERT_EQ(target->events.size(), 1u);
  EXPECT_EQ(target->events[0].source,
            SyntheticPointerEventRecord::Source::kMouse);
  EXPECT_EQ(target->events[0].type, "mouseWheel");
  EXPECT_FLOAT_EQ(target->events[0].delta_x, -1.5f);
  EXPECT_FLOAT_EQ(target->events[0].delta_y, 2.5f);
}

TEST_F(LynxViewEventSimulationProxyTest, EmulateMouseForwardsUnchanged) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  for (const auto* type : {"mousePressed", "mouseMoved", "mouseReleased"}) {
    proxy.EmulateMouse(type, 100, 200, "left", 0, 0, 4, 1);
  }

  ASSERT_EQ(target->events.size(), 3u);
  EXPECT_EQ(target->events[0].type, "mousePressed");
  EXPECT_EQ(target->events[1].type, "mouseMoved");
  EXPECT_EQ(target->events[2].type, "mouseReleased");
  for (const auto& event : target->events) {
    EXPECT_EQ(event.source, SyntheticPointerEventRecord::Source::kMouse);
    EXPECT_EQ(event.x, 100);
    EXPECT_EQ(event.y, 200);
    EXPECT_EQ(event.button, "left");
    EXPECT_EQ(event.modifiers, 4);
    EXPECT_EQ(event.click_count, 1);
    EXPECT_FLOAT_EQ(event.delta_x, 0);
    EXPECT_FLOAT_EQ(event.delta_y, 0);
  }
}

TEST_F(LynxViewEventSimulationProxyTest, EmulateMouseWheelPreservesDirection) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateMouse("mouseWheel", 7, 8, "none", 1.5f, -2.5f, 4, 0);

  ASSERT_EQ(target->events.size(), 1u);
  const auto& event = target->events[0];
  EXPECT_EQ(event.source, SyntheticPointerEventRecord::Source::kMouse);
  EXPECT_EQ(event.type, "mouseWheel");
  EXPECT_EQ(event.x, 7);
  EXPECT_EQ(event.y, 8);
  EXPECT_EQ(event.button, "none");
  EXPECT_EQ(event.modifiers, 4);
  EXPECT_EQ(event.click_count, 0);
  EXPECT_FLOAT_EQ(event.delta_x, 1.5f);
  EXPECT_FLOAT_EQ(event.delta_y, -2.5f);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx

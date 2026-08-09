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

// cspell:ignore rightmousedown rightmousemove rightmouseup
struct TouchEventRecord {
  std::string name;
  int tag;
  int x;
  int y;
};

struct MouseEventRecord {
  std::string name;
  float x;
  float y;
  float delta_x;
  float delta_y;
};

class FakeEventSimulationTarget : public LynxViewEventSimulationTarget {
 public:
  explicit FakeEventSimulationTarget(int node_for_location)
      : node_for_location_(node_for_location) {}

  int GetNodeForLocation(int x, int y) override {
    hit_test_requests.emplace_back(x, y);
    return node_for_location_;
  }

  void SendTouchEvent(const std::string& name, int tag, int x, int y) override {
    touch_events.push_back({name, tag, x, y});
  }

  void EmulateMouseEvent(const std::string& event_name, float x, float y,
                         float delta_x, float delta_y) override {
    mouse_events.push_back({event_name, x, y, delta_x, delta_y});
  }

  void Focus(int node_id) override { focused_node_ids.push_back(node_id); }

  void InsertText(const std::string& text) override {
    inserted_texts.push_back(text);
  }

  std::vector<std::pair<int, int>> hit_test_requests;
  std::vector<TouchEventRecord> touch_events;
  std::vector<MouseEventRecord> mouse_events;
  std::vector<int> focused_node_ids;
  std::vector<std::string> inserted_texts;

 private:
  int node_for_location_;
};

class LynxViewEventSimulationProxyTest : public testing::Test {
 protected:
  LynxViewEventSimulationProxy CreateProxy(
      FakeEventSimulationTarget** out_target, int node_for_location = 42) {
    auto target =
        std::make_unique<FakeEventSimulationTarget>(node_for_location);
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

TEST_F(LynxViewEventSimulationProxyTest,
       PressMoveReleaseBeyondTapSlopSkipsTap) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mousePressed", 100, 200, "left", 0, 0, 0, 1);
  proxy.EmulateTouch("mouseMoved", 110, 200, "left", 0, 0, 0, 1);
  proxy.EmulateTouch("mouseReleased", 110, 200, "left", 0, 0, 0, 1);

  ASSERT_EQ(target->hit_test_requests.size(), 1u);
  EXPECT_EQ(target->hit_test_requests[0], std::make_pair(100, 200));
  ASSERT_EQ(target->touch_events.size(), 3u);
  EXPECT_EQ(target->touch_events[0].name, "touchstart");
  EXPECT_EQ(target->touch_events[1].name, "touchmove");
  EXPECT_EQ(target->touch_events[2].name, "touchend");
  EXPECT_EQ(target->touch_events[0].tag, 42);
}

TEST_F(LynxViewEventSimulationProxyTest, ReleaseWithinTapSlopSendsTap) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mousePressed", 10, 20, "left", 0, 0, 0, 1);
  proxy.EmulateTouch("mouseReleased", 13, 24, "left", 0, 0, 0, 1);

  ASSERT_EQ(target->touch_events.size(), 3u);
  EXPECT_EQ(target->touch_events[0].name, "touchstart");
  EXPECT_EQ(target->touch_events[1].name, "touchend");
  EXPECT_EQ(target->touch_events[2].name, "tap");
}

TEST_F(LynxViewEventSimulationProxyTest, InvalidHitTestDoesNotSendEvents) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target, 0);

  proxy.EmulateTouch("mousePressed", 1, 2, "left", 0, 0, 0, 1);
  proxy.EmulateTouch("mouseReleased", 1, 2, "left", 0, 0, 0, 1);

  ASSERT_EQ(target->hit_test_requests.size(), 1u);
  EXPECT_TRUE(target->touch_events.empty());
  EXPECT_TRUE(target->mouse_events.empty());
}

TEST_F(LynxViewEventSimulationProxyTest, RightClickUsesMousePath) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mousePressed", 5, 6, "right", 0, 0, 0, 1);
  proxy.EmulateTouch("mouseMoved", 7, 8, "right", 0, 0, 0, 1);
  proxy.EmulateTouch("mouseReleased", 7, 8, "right", 0, 0, 0, 1);

  EXPECT_TRUE(target->touch_events.empty());
  ASSERT_EQ(target->mouse_events.size(), 3u);
  EXPECT_EQ(target->mouse_events[0].name, "rightmousedown");
  EXPECT_EQ(target->mouse_events[1].name, "rightmousemove");
  EXPECT_EQ(target->mouse_events[2].name, "rightmouseup");
}

TEST_F(LynxViewEventSimulationProxyTest, WheelUsesMousePath) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mouseWheel", 7, 8, "left", 1.5f, -2.5f, 0, 0);

  EXPECT_TRUE(target->touch_events.empty());
  ASSERT_EQ(target->mouse_events.size(), 1u);
  EXPECT_EQ(target->mouse_events[0].name, "wheel");
  EXPECT_FLOAT_EQ(target->mouse_events[0].delta_x, 1.5f);
  EXPECT_FLOAT_EQ(target->mouse_events[0].delta_y, -2.5f);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx

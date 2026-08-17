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
  void DispatchSyntheticPointerEvent(const std::string& event_type, int x,
                                     int y, const std::string& button,
                                     float delta_x, float delta_y,
                                     int modifiers, int click_count) override {
    pointer_events.push_back(
        {event_type, x, y, button, delta_x, delta_y, modifiers, click_count});
  }

  void Focus(int node_id) override { focused_node_ids.push_back(node_id); }

  void InsertText(const std::string& text) override {
    inserted_texts.push_back(text);
  }

  std::vector<SyntheticPointerEventRecord> pointer_events;
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

TEST_F(LynxViewEventSimulationProxyTest, ForwardsPointerInputUnchanged) {
  FakeEventSimulationTarget* target = nullptr;
  auto proxy = CreateProxy(&target);

  proxy.EmulateTouch("mousePressed", 100, 200, "left", 0, 0, 4, 1);
  proxy.EmulateTouch("mouseMoved", 110, 190, "left", 0, 0, 4, 0);
  proxy.EmulateTouch("mouseReleased", 110, 190, "left", 0, 0, 4, 1);
  proxy.EmulateTouch("mouseWheel", 7, 8, "none", 1.5f, -2.5f, 0, 0);

  ASSERT_EQ(target->pointer_events.size(), 4u);
  EXPECT_EQ(target->pointer_events[0].type, "mousePressed");
  EXPECT_EQ(target->pointer_events[0].x, 100);
  EXPECT_EQ(target->pointer_events[0].y, 200);
  EXPECT_EQ(target->pointer_events[0].button, "left");
  EXPECT_EQ(target->pointer_events[0].modifiers, 4);
  EXPECT_EQ(target->pointer_events[0].click_count, 1);
  EXPECT_EQ(target->pointer_events[1].type, "mouseMoved");
  EXPECT_EQ(target->pointer_events[2].type, "mouseReleased");
  EXPECT_EQ(target->pointer_events[3].type, "mouseWheel");
  EXPECT_FLOAT_EQ(target->pointer_events[3].delta_x, 1.5f);
  EXPECT_FLOAT_EQ(target->pointer_events[3].delta_y, -2.5f);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx

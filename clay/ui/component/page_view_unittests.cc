// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>

#include "clay/ui/component/page_view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

class TestEventDelegate : public EventDelegate {
 public:
  explicit TestEventDelegate(bool default_prevented)
      : default_prevented_(default_prevented) {}

  void OnTouchEvent(const std::string&, int, float, float, float,
                    float) override {}
  void OnMouseEvent(const std::string&, int, int, int, float, float, float,
                    float, float) override {}
  void OnWheelEvent(const std::string&, int, float, float, float, float, float,
                    float) override {}
  void OnKeyEvent(const std::string&, int, const char*, bool) override {}
  void OnGestureHandlerEvent(const std::string&, int, uint32_t, float, float,
                             float, float, int64_t, Value&) override {}
  void OnAnimationEvent(const std::string&, const char*, int) override {}
  void OnTransitionEvent(const std::string&, const char*, int,
                         ClayAnimationPropertyType) override {}
  void OnFocusChanged(int, bool) override {}
  void OnHoverChanged(int, bool) override {}
  void OnDragDropEvent(const std::string&, int, Value::Map) override {}
  void OnViewportMetricsChanged(double, double, double, double, double, double,
                                double, bool) override {}
  void OnDrawEndEvent() override {}
  void OnSendCustomEvent(int, const std::string&, Value::Map) override {}
  void OnSendGlobalEvent(const std::string&, Value) override {}
  void OnFirstMeaningfulPaint() override {}
  void OnOverlayEvent(int, const char*, int, const char**,
                      const char*) override {}
  void OnLayoutChanged(int, Value::Map) override {}
  void OnIntersectionEvent(int, Value::Map) override {}
  void OnCallJSApiCallback(int, Value) override {}
  void CallJSIntersectionObserver(int, int, Value) override {}

  bool OnSendCancelableCustomEvent(int view_id,
                                   const std::string& event_name,
                                   Value::Map args) override {
    received_cancelable_event = true;
    received_view_id = view_id;
    received_event_name = event_name;
    received_args_size = args.size();
    return default_prevented_;
  }

  bool received_cancelable_event = false;
  int received_view_id = 0;
  std::string received_event_name;
  size_t received_args_size = 0;

 private:
  bool default_prevented_;
};

}  // namespace

TEST(PageViewTest, EmptyKeyframesData) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);
  Value keyframes_data;
  page_view->SetKeyframesData(keyframes_data);
  EXPECT_EQ(page_view->GetKeyframesMap("name"), nullptr);
}

TEST(PageViewTest, KeyframesData) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  Value keyframes_data = Value{
      {"anim_1",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
      {"anim_2",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
      {"anim_3",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
  };

  page_view->SetKeyframesData(keyframes_data);

  auto check_keyframes_map = [&page_view](const char* anim_name) {
    const KeyframesMap* ret = page_view->GetKeyframesMap(anim_name);
    EXPECT_TRUE(ret);
    auto it = ret->find(ClayAnimationPropertyType::kBackgroundColor);
    EXPECT_TRUE(it != ret->end());
    it = ret->find(ClayAnimationPropertyType::kOpacity);
    EXPECT_TRUE(it != ret->end());
  };

  check_keyframes_map("anim_1");
  check_keyframes_map("anim_2");
  check_keyframes_map("anim_3");

  Value keyframes_data_2 = Value{
      {"anim_1",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
  };
  page_view->SetKeyframesData(keyframes_data_2);

  check_keyframes_map("anim_1");
}

TEST(PageViewTest, SendCancelableCustomEventWithoutDelegateReturnsFalse) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  EXPECT_FALSE(page_view->SendCancelableCustomEvent(
      101, "beforeinput", Value::Map{{"inputType", Value("insertText")}}));
}

TEST(PageViewTest, SendCancelableCustomEventUsesDelegateResult) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);
  TestEventDelegate delegate(/*default_prevented=*/true);
  page_view->SetEventDelegate(&delegate);

  EXPECT_TRUE(page_view->SendCancelableCustomEvent(
      101, "beforeinput", Value::Map{{"inputType", Value("insertText")}}));
  EXPECT_TRUE(delegate.received_cancelable_event);
  EXPECT_EQ(delegate.received_view_id, 101);
  EXPECT_EQ(delegate.received_event_name, "beforeinput");
  EXPECT_EQ(delegate.received_args_size, 1u);
}

TEST(PageViewTest, SendCancelableCustomEventUsesDelegateFalseResult) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);
  TestEventDelegate delegate(/*default_prevented=*/false);
  page_view->SetEventDelegate(&delegate);

  EXPECT_FALSE(page_view->SendCancelableCustomEvent(
      101, "beforeinput", Value::Map{{"inputType", Value("insertText")}}));
  EXPECT_TRUE(delegate.received_cancelable_event);
}

}  // namespace clay

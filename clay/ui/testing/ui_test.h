// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_TESTING_UI_TEST_H_
#define CLAY_UI_TESTING_UI_TEST_H_

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/thread.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/render_delegate.h"
#include "clay/ui/testing/test_utils.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkSize.h"

namespace clay {

class UITest : public ::testing::Test {
 public:
  PointerEvent CreatePointer(int pointer_id, PointerEvent::EventType type,
                             FloatPoint position, FloatPoint delta = {},
                             int64_t timestamp = 0);
  void DispatchTapEvent(FloatPoint point);
  void DispatchDragEvent(FloatPoint start, FloatPoint end, bool fling = false,
                         float steps = 10,
                         float interval_ms = 1000.0f / 120.0f);
  void DispatchTouchPadEvent(FloatPoint from, FloatPoint to, int event_num);

  void Layout();

  // Currently some animations rely on the system time (e.g.
  // `Scroller::start_time_`). So we should reset the last_frame_time before
  // starting a new animation.
  void ResetAnimationTime();

  void DoAnimation(int ms = 1000);

  void InvokeUIMethod(
      BaseView* view, const std::string& method_name,
      std::initializer_list<std::pair<std::string, clay::Value&&>> params,
      std::function<void(LynxUIMethodResult code, const clay::Value& data)>
          callback);

  fml::RefPtr<fml::TaskRunner> ui_task_runner() {
    return ui_thread_->GetTaskRunner();
  }

  // This is convenience for verifying component events.
  std::function<void(int, const char*, clay::Value::Map)>
      custom_event_callback_;
  MOCK_METHOD(void, OnCustomEvent,
              (std::string event_name, const clay::Value::Map& params), ());

 protected:
  virtual void UISetUp() {}
  virtual void UITearDown() {}
  void SetUp() override;
  void TearDown() override;

  void AsyncStart();
  void AsyncEnd();
  void AsyncWait();

  BaseView* ViewForId(const std::string& id_selector);
  void NavigateByDirection(FocusManager::Direction direction);

  std::unique_ptr<PageView> page_;

 private:
  std::unique_ptr<fml::Thread> ui_thread_;
  std::unique_ptr<RenderDelegate> delegate_;
  std::unique_ptr<EventDelegate> event_delegate_;
  int async_level_ = 0;
  fml::AutoResetWaitableEvent body_latch_;
};

#ifndef GTEST_TEST_CLASS_NAME_
#define GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) \
  test_suite_name##_##test_name##_Test
#endif

#define TEST_F_UI(test_fixture, test_name)                                   \
  class GTEST_TEST_CLASS_NAME_(test_fixture, test_name)                      \
      : public test_fixture {                                                \
   public:                                                                   \
    GTEST_TEST_CLASS_NAME_(test_fixture, test_name)() {}                     \
                                                                             \
   private:                                                                  \
    virtual void TestBody() {                                                \
      AsyncStart();                                                          \
      ui_task_runner()->PostTask([&]() {                                     \
        TestBodyUI();                                                        \
        AsyncEnd();                                                          \
      });                                                                    \
      AsyncWait();                                                           \
    }                                                                        \
    void TestBodyUI();                                                       \
    static ::testing::TestInfo* const test_info_ GTEST_ATTRIBUTE_UNUSED_;    \
    BASE_DISALLOW_COPY_AND_ASSIGN(GTEST_TEST_CLASS_NAME_(test_fixture,       \
                                                         test_name));        \
  };                                                                         \
                                                                             \
  ::testing::TestInfo* const GTEST_TEST_CLASS_NAME_(test_fixture,            \
                                                    test_name)::test_info_ = \
      ::testing::internal::MakeAndRegisterTestInfo(                          \
          #test_fixture, #test_name, nullptr, nullptr,                       \
          ::testing::internal::CodeLocation(__FILE__, __LINE__),             \
          (::testing::internal::GetTypeId<test_fixture>()),                  \
          ::testing::internal::SuiteApiResolver<                             \
              test_fixture>::GetSetUpCaseOrSuite(__FILE__, __LINE__),        \
          ::testing::internal::SuiteApiResolver<                             \
              test_fixture>::GetTearDownCaseOrSuite(__FILE__, __LINE__),     \
          new ::testing::internal::TestFactoryImpl<GTEST_TEST_CLASS_NAME_(   \
              test_fixture, test_name)>);                                    \
  void GTEST_TEST_CLASS_NAME_(test_fixture, test_name)::TestBodyUI()

}  // namespace clay
#endif  // CLAY_UI_TESTING_UI_TEST_H_

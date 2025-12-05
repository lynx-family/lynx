// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/testing/ui_test.h"

#include "clay/gfx/testing_utils.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/resource/font_collection.h"

namespace clay {

class MockDelegate : public clay::RenderDelegate {
 public:
  void ScheduleFrame() override {}
  void ForceBeginFrame() override {}
  void OnFirstMeaningfulLayout() override {}

  bool Raster(std::unique_ptr<clay::LayerTree> layer_tree,
              std::unique_ptr<clay::FrameTimingsRecorder> recorder,
              bool force) override {
    return false;
  }
  void ShowSoftInput(int type, int action) override {}
  void HideSoftInput() override {}
  std::string ShouldInterceptUrl(const std::string& origin_url,
                                 bool should_decode) override {
    return "";
  }

  void MakeRasterSnapshot(
      std::unique_ptr<LayerTree> layer_tree,
      std::function<void(fml::RefPtr<DlImage>)> callback) override {}
  fml::RefPtr<PaintImage> MakeRasterSnapshot(
      sk_sp<SkPicture> picture, skity::Vec2 picture_size) override {
    return nullptr;
  }
  void SetClipboardData(const std::u16string& data) override {}
  std::u16string GetClipboardData() override { return {}; }
#if defined(OS_WIN) || defined(OS_MAC)
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type) override {}
  void ClearTextInputClient() override {}
  void SetEditableTransform(const float transform_matrix[16]) override {}
  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const std::string& selection_affinity,
                       const std::string& text, bool selection_directional,
                       uint64_t selection_extent,
                       uint64_t composing_base) override {}
  void SetCaretRect(float x, float y, float width, float height) override {}
  void setMarkedTextRect(float x, float y, float width, float height) override {
  }
  void ShowTextInput() override {}
  void HideTextInput() override {}
  void WindowMove() override {}
  void ActivateSystemCursor(int type, const std::string& path) override {}
#endif
  void ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                    const std::string& flag) override {}
  BaseView* FindViewById(int view_id) override { return nullptr; }
  ShadowNode* FindShadowNodeById(int node_id) override { return nullptr; }

  void RegisterDrawableImage(
      std::shared_ptr<DrawableImage> drawable_image) override {}
  void UnregisterDrawableImage(int64_t id) override {}
};

class MockEventDelegate : public clay::EventDelegate {
 public:
  MockEventDelegate(UITest* uitest) : uitest_(uitest) {}

  void OnTouchEvent(const std::string& event_name, int tag, float x, float y,
                    float page_x, float page_y) override {}
  void OnMouseEvent(const std::string& event_name, int view_id, int button,
                    int buttons, float scale, float x, float y, float page_x,
                    float page_y) override {}
  void OnWheelEvent(const std::string& event_name, int view_id, float x,
                    float y, float page_x, float page_y, float delta_x,
                    float delta_y) override {}
  void OnKeyEvent(const std::string& event_name, int view_id, const char* key,
                  bool repeat) override {}
  void OnAnimationEvent(const std::string& event_name,
                        const char* animation_name, int view_id) override {}
  void OnTransitionEvent(const std::string& event_name,
                         const char* animation_name, int view_id,
                         ClayAnimationPropertyType type) override {}
  void OnFocusChanged(int view_id, bool focus) override {}
  void OnHoverChanged(int view_id, bool hover) override {}
  void OnDragDropEvent(const std::string& event_name, int view_id,
                       clay::Value::Map map) override {}
  void OnViewportMetricsChanged(double device_pixel_ratio,
                                double device_density_dpi, double logical_width,
                                double logical_height,
                                double physical_screen_width,
                                double physical_screen_height,
                                double font_scale, bool night_mode) override {}
  void OnSendGlobalEvent(const std::string& event_name,
                         clay::Value args) override {}
  void OnDrawEndEvent() override {}
  void OnFirstMeaningfulPaint() override {}
  void OnOverlayEvent(int view_id, const char* overlay_id, int overlay_count,
                      const char** overlay_ids,
                      const char* event_name) override {}
  void OnLayoutChanged(int view_id, clay::Value::Map map) override {}
  void OnIntersectionEvent(int view_id, clay::Value::Map map) override {}
  void OnCallJSApiCallback(int callback_id, clay::Value value) override {}
  void CallJSIntersectionObserver(int observer_id, int callback_id,
                                  clay::Value params) override {}

  void OnSendCustomEvent(int view_id, const std::string& event_name,
                         clay::Value::Map args) override {
    if (uitest_->custom_event_callback_) {
      uitest_->custom_event_callback_(view_id, event_name.c_str(),
                                      std::move(args));
    }
  }

 private:
  UITest* uitest_;
};

void UITest::DispatchDragEvent(FloatPoint start, FloatPoint end, bool fling,
                               float steps, float interval_ms) {
  int64_t interval = interval_ms * 1000;
  int64_t timestamp = fml::TimePoint::Now().ToEpochDelta().ToMicroseconds();
  page_->DispatchPointerEvent({CreatePointer(
      0, PointerEvent::EventType::kDownEvent, start, {}, timestamp)});
  FloatPoint last = start;
  for (int i = 1; i <= steps; i++) {
    timestamp += interval;
    auto point = FloatPoint::Lerp(start, end, static_cast<float>(i) / steps);
    page_->DispatchPointerEvent(
        {CreatePointer(0, PointerEvent::EventType::kMoveEvent, point,
                       point - last, timestamp)});
    last = point;
  }
  if (!fling) {
    // Stop at the end point for 20 samples to ensure the calculated velocity
    // is 0. See `VelocityTracker::GetVelocityEstimate`
    for (int i = 0; i < 20; i++) {
      timestamp += interval;
      page_->DispatchPointerEvent({CreatePointer(
          0, PointerEvent::EventType::kMoveEvent, end, end - last, timestamp)});
    }
  }
  timestamp += interval;
  page_->DispatchPointerEvent({CreatePointer(
      0, PointerEvent::EventType::kUpEvent, end, {}, timestamp)});
}

void UITest::Layout() { page_->GetLayoutController()->Layout(); }

void UITest::ResetAnimationTime() {
  page_->GetAnimationHandler()->DoAnimationFrame(
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds());
}

void UITest::DoAnimation(int ms) {
  auto last_frame_time =
      page_->GetAnimationHandler()->GetCurrentAnimationTime();
  if (last_frame_time < 0) {
    last_frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
    // The animation will be started in the next frame.
    // So we should trigger an extra `DoAnimationFrame`
    page_->GetAnimationHandler()->DoAnimationFrame(last_frame_time);
  }
  // In the ui test, the start_time_ in scroller may not be initialized,we need
  // do a additional DoAnimationFrame to initialize scroller (test only).
  page_->GetAnimationHandler()->DoAnimationFrame(last_frame_time);
  page_->GetAnimationHandler()->DoAnimationFrame(last_frame_time + ms);
}

void UITest::InvokeUIMethod(
    BaseView* view, const std::string& method_name,
    std::initializer_list<std::pair<std::string, clay::Value&&>> params,
    std::function<void(LynxUIMethodResult code, const clay::Value& data)>
        callback) {
  LynxModuleValues values;
  for (auto& pair : params) {
    values.names.push_back(pair.first);
    values.values.push_back(std::move(pair.second));
  }
  LynxUIMethodCallback wrapped_callback = [callback](LynxUIMethodResult code,
                                                     const clay::Value& data) {
    if (callback) {
      callback(code, data);
    }
  };
  LynxUIMethodRegistrar::Instance().Invoke(method_name, view, values,
                                           wrapped_callback);
}

void UITest::SetUp() {
  ui_thread_ = std::make_unique<fml::Thread>("ui");
  delegate_ = std::make_unique<MockDelegate>();
  event_delegate_ = std::make_unique<MockEventDelegate>(this);
  auto font_collection = FontCollection::Instance();
  font_collection->SetupDefaultFontManager(0);
  fml::AutoResetWaitableEvent latch;
  ui_task_runner()->PostTask([&]() {
    page_ = std::make_unique<PageView>(0, nullptr, ui_thread_->GetTaskRunner());
    page_->SetEventDelegate(event_delegate_.get());
    page_->SetRenderDelegate(delegate_.get());
    page_->SetBound(0, 0, 1000, 1000);
    custom_event_callback_ = [this](int id, const char* event_name,
                                    clay::Value::Map map) {
      OnCustomEvent(event_name, std::move(map));
    };
    page_->GetFocusManager()->preference().switch_up_to_once_per_frame = false;
    UISetUp();
    latch.Signal();
  });
  latch.Wait();
}

void UITest::TearDown() {
  fml::AutoResetWaitableEvent latch;
  ui_task_runner()->PostTask([&]() {
    UITearDown();
    page_.reset();
    latch.Signal();
  });
  latch.Wait();
}

void UITest::AsyncStart() { async_level_++; }

void UITest::AsyncEnd() {
  async_level_--;
  if (async_level_ == 0) {
    body_latch_.Signal();
  }
}

void UITest::AsyncWait() {
  if (async_level_ > 0) {
    body_latch_.Wait();
  }
}

BaseView* UITest::ViewForId(const std::string& id_selector) {
  return ViewContext::FindViewByIdSelector(id_selector, page_.get());
}

void UITest::NavigateByDirection(FocusManager::Direction direction) {
  PhysicalKeyboardKey physical = PhysicalKeyboardKey::kUnknown;
  LogicalKeyboardKey logical = LogicalKeyboardKey::kUnknown;
  switch (direction) {
    case FocusManager::Direction::kLeft:
      physical = PhysicalKeyboardKey::kArrowLeft;
      logical = LogicalKeyboardKey::kArrowLeft;
      break;
    case FocusManager::Direction::kRight:
      physical = PhysicalKeyboardKey::kArrowRight;
      logical = LogicalKeyboardKey::kArrowRight;
      break;
    case FocusManager::Direction::kUp:
      physical = PhysicalKeyboardKey::kArrowUp;
      logical = LogicalKeyboardKey::kArrowUp;
      break;
    case FocusManager::Direction::kDown:
      physical = PhysicalKeyboardKey::kArrowDown;
      logical = LogicalKeyboardKey::kArrowDown;
      break;
    default:
      FML_DCHECK(false) << "Unhandled key for NavigateByDirection";
      return;
  }
  page_->DispatchKeyEvent(
      std::make_unique<KeyEvent>(
          fml::TimePoint::Now().ToEpochDelta().ToMilliseconds(),
          KeyEventType::kDown, physical, logical, false, ""),
      [](bool) {});

  page_->DispatchKeyEvent(
      std::make_unique<KeyEvent>(
          fml::TimePoint::Now().ToEpochDelta().ToMilliseconds(),
          KeyEventType::kUp, physical, logical, false, ""),
      [](bool) {});
}

PointerEvent UITest::CreatePointer(int pointer_id, PointerEvent::EventType type,
                                   FloatPoint position, FloatPoint delta,
                                   int64_t timestamp) {
  PointerEvent pointer(type);
  pointer.buttons = PointerEvent::kPrimary;
  pointer.pointer_id = pointer_id;
  pointer.position = position;
  pointer.delta = FloatSize(delta.x(), delta.y());
  pointer.timestamp = timestamp;
  return pointer;
}

void UITest::DispatchTapEvent(FloatPoint point) {
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, point)});
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kUpEvent, point)});
}

void UITest::DispatchTouchPadEvent(FloatPoint from, FloatPoint to,
                                   int event_num) {
  std::vector<PointerEvent> events;
  float x_stride = (to.x() - from.x()) / event_num;
  float y_stride = (to.y() - from.y()) / event_num;
  for (int i = 1; i < event_num + 1; i++) {
    auto temp_event = PointerEvent(PointerEvent::EventType::kSignalEvent);
    temp_event.device = PointerEvent::kMouse;
    temp_event.signal_kind = PointerEvent::SignalKind::kScroll;
    temp_event.scroll_delta_x = x_stride;
    temp_event.scroll_delta_y = y_stride;
    temp_event.position = {from.x() + i * temp_event.scroll_delta_x,
                           from.y() + i * temp_event.scroll_delta_y};
    events.push_back(temp_event);
  }
  page_->DispatchPointerEvent(events);
}

}  // namespace clay

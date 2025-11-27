// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_view_service_desktop.h"

#include <assert.h>

#include <memory>
#include <unordered_set>
#include <utility>

#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"

#if OS_WIN
#include <Windows.h>
#elif OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

namespace clay {

class NativeViewPluginDesktop : public NativeViewPlugin {
 public:
  NativeViewPluginDesktop(int id, NativeView* native_view,
                          NativePlatformView* native_platform_view)
      : NativeViewPlugin(id),
        native_view_(native_view),
        native_platform_view_(native_platform_view) {}
  ~NativeViewPluginDesktop() override { native_platform_view_->Release(); }

  bool OnCreate(std::string tag) override {
    return native_platform_view_->OnCreate();
  }
  void OnAttach() override { native_platform_view_->OnAttach(); }
  void OnDetach() override { native_platform_view_->OnDetach(); }
  void OnDestroy() override { native_platform_view_->OnDestroy(); }
  void LayoutChanged(float left, float top, float width,
                     float height) override {
    float pixel_ratio = native_view_->page_view()->DevicePixelRatio();
    native_platform_view_->OnLayoutChanged(left, top, width, height,
                                           pixel_ratio);
  }
  void UpdatePlatformAttributes(const clay::Value::Map& attrs,
                                const clay::Value::Array& events) override {
    if (attrs.empty() && events.empty()) {
      return;
    }
    lynx_api_env env = nullptr;
    lynx_value lynx_attrs = lynx::ValueConverter::CreateLynxValue(env, attrs);
    lynx_value lynx_events = lynx::ValueConverter::CreateLynxValue(env, events);
    native_platform_view_->OnPropertiesChanged(lynx_attrs, lynx_events);
    lynx_value_remove_reference(env, lynx_attrs, nullptr);
    lynx_value_remove_reference(env, lynx_events, nullptr);
  }
  void InvokePlatformMethod(
      const std::string& method, const clay::Value::Map& attrs,
      const clay::LynxUIMethodCallback& callback) override {
    lynx_api_env env = nullptr;
    lynx_value lynx_attrs = lynx::ValueConverter::CreateLynxValue(env, attrs);
    native_platform_view_->OnMethodInvoked(
        method, lynx_attrs,
        [env, cb = std::move(callback)](int code, lynx_value data) {
          clay::Value result = lynx::ValueConverter::CreateClayValue(env, data);
          lynx_value_remove_reference(env, data, nullptr);
          cb(static_cast<LynxUIMethodResult>(code), std::move(result));
        });
    lynx_value_remove_reference(env, lynx_attrs, nullptr);
  }

  std::string GetName() const override { return native_view_->GetName(); }
  bool SupportScrolling() override {
    return native_platform_view_->SupportScrolling();
  }
  fml::RefPtr<SharedImageSink> GetSharedImageSink() override {
    return fml::RefPtr<SharedImageSink>(reinterpret_cast<SharedImageSink*>(
        native_platform_view_->GetSharedImageSink()));
  }

  static_assert(static_cast<int>(kNativeEventPanZoomEnd) ==
                static_cast<int>(PointerEvent::EventType::kPanZoomEndEvent));
  static_assert(static_cast<int>(kNativeDeviceTrackpad) ==
                static_cast<int>(PointerEvent::DeviceType::kTrackpad));
  static_assert(static_cast<int>(kNativeSignalEndScroll) ==
                static_cast<int>(PointerEvent::SignalKind::kEndScroll));
  static_assert(static_cast<int>(kNativeMouseButtonForward) ==
                static_cast<int>(PointerEvent::MouseButton::kForward));

  void OnTouchEvent(const clay::PointerEvent& event,
                    const clay::FloatPoint& transformed_postion) override {
    FloatPoint position =
        native_view_->page_view()->ConvertTo<kPixelTypeLogical>(event.position);
    native_view_motion_event_t motion_event;
    memset(&motion_event, 0, sizeof(motion_event));
    motion_event.timestamp = event.timestamp;
    motion_event.type = static_cast<lynx_native_event_type_t>(event.type);
    motion_event.device = static_cast<lynx_native_device_type_t>(event.device);
    motion_event.signal_kind =
        static_cast<lynx_native_signal_kind_t>(event.signal_kind);
    motion_event.pointer_id = event.pointer_id;
    motion_event.device_id = event.device_id;
    motion_event.buttons = event.buttons;
    motion_event.x = transformed_postion.x();
    motion_event.y = transformed_postion.y();
    motion_event.pageX = position.x();
    motion_event.pageY = position.y();
    if (event.signal_kind != PointerEvent::SignalKind::kNone) {
      motion_event.deltaX = event.scroll_delta_x;
      motion_event.deltaY = event.scroll_delta_y;
    } else if (event.type == PointerEvent::EventType::kPanZoomUpdateEvent) {
      motion_event.deltaX = event.pan_delta.width();
      motion_event.deltaY = event.pan_delta.height();
    } else {
      motion_event.deltaX = event.delta.width();
      motion_event.deltaY = event.delta.height();
    }
    motion_event.scale = event.scale;
#if OS_WIN
    if ((GetKeyState(VK_LMENU) | GetKeyState(VK_RMENU)) & 0x8000) {
      motion_event.altKey = true;
    }
    if ((GetKeyState(VK_LCONTROL) | GetKeyState(VK_RCONTROL)) & 0x8000) {
      motion_event.ctrlKey = true;
    }
    if ((GetKeyState(VK_LSHIFT) | GetKeyState(VK_RSHIFT)) & 0x8000) {
      motion_event.shiftKey = true;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
      motion_event.metaKey = true;
    }
#elif OS_MAC
    if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Option) ||
        CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                              kVK_RightOption)) {
      motion_event.altKey = true;
    }
    if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Control) ||
        CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                              kVK_RightControl)) {
      motion_event.ctrlKey = true;
    }
    if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Shift) ||
        CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                              kVK_RightShift)) {
      motion_event.shiftKey = true;
    }
    if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Command) ||
        CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                              kVK_RightCommand)) {
      motion_event.metaKey = true;
    }
#endif
    if (native_platform_view_->HandleMotionEvent(&motion_event)) {
      return;
    }

    int modifiers =
        event.buttons & (PointerEvent::kPrimary | PointerEvent::kSecondary |
                         PointerEvent::kMiddle);
    if (event.signal_kind == PointerEvent::SignalKind::kScroll) {
      native_platform_view_->OnMouseWheelEvent(
          transformed_postion.x(), transformed_postion.y(), modifiers,
          event.scroll_delta_x, -event.scroll_delta_y);
      return;
    }
    if (event.type == PointerEvent::EventType::kPanZoomUpdateEvent) {
      native_platform_view_->OnMouseWheelEvent(
          transformed_postion.x(), transformed_postion.y(), modifiers,
          event.pan_delta.width(), -event.pan_delta.height());
      return;
    }
    switch (event.type) {
      case PointerEvent::EventType::kDownEvent:
      case PointerEvent::EventType::kUpEvent:
        native_platform_view_->OnMouseClickEvent(
            transformed_postion.x(), transformed_postion.y(), event.buttons,
            event.type == PointerEvent::EventType::kUpEvent);
        break;
      default:
      case PointerEvent::EventType::kHoverEvent:
        native_platform_view_->OnMouseMoveEvent(
            transformed_postion.x(), transformed_postion.y(), modifiers, false);
        break;
    }
  }
  void OnFocusChanged(bool focused, bool is_leaf) override {
    native_platform_view_->OnFocusChanged(focused, is_leaf);
  }
  clay::MeasureResult Measure(
      const clay::MeasureConstraint& constraint) override {
    return {};
  }

 private:
  NativeView* native_view_;
  NativePlatformView* native_platform_view_;
};

std::unique_ptr<NativeViewPlugin>
NativeViewServiceDesktop::CreateNativeViewPlugin(int id, NativeView* view_ptr) {
  const auto& tag = view_ptr->GetName();
  auto search = view_factories.find(tag);
  if (search != view_factories.end()) {
    auto native_view_ptr = search->second();
    if (native_view_ptr) {
      native_view_ptr->native_view_ = view_ptr;
      return std::make_unique<NativeViewPluginDesktop>(id, view_ptr,
                                                       native_view_ptr);
    }
  }
  return nullptr;
}

// static
void NativeViewServiceDesktop::SetViewFactories(
    ViewContext* view_context,
    std::unordered_map<std::string, std::function<NativePlatformView*()>>
        factories) {
  std::unordered_set<std::string> native_view_tags;
  for (auto& pair : factories) {
    native_view_tags.insert(pair.first);
  }
  view_context->SyncNativeViewTags(native_view_tags);

  Puppet<Owner::kUI, NativeViewService> native_view_service =
      view_context->GetPageView()
          ->GetServiceManager()
          ->GetService<NativeViewService>();
  native_view_service.Act([&factories](auto& service) {
    static_cast<NativeViewServiceDesktop&>(service).view_factories =
        std::move(factories);
  });
}

// static
void NativeViewServiceDesktop::SetViewFactories(
    ViewContext* view_context,
    std::unordered_map<std::string, std::pair<lynx_native_view_creator, void*>>
        creators) {
  std::unordered_map<std::string, clay::NativePlatformView::Creator>
      view_factories;
  for (auto& pair : creators) {
    view_factories[pair.first] =
        [native_view_creator = pair.second]() -> clay::NativePlatformView* {
      auto creator = reinterpret_cast<clay::NativePlatformView* (*)(void*)>(
          std::get<0>(native_view_creator));
      auto opaque = std::get<1>(native_view_creator);
      return creator(opaque);
    };
  }
  SetViewFactories(view_context, view_factories);
}

// static
void NativeViewServiceDesktop::AddViewFactory(ViewContext* view_context,
                                              const char* name,
                                              lynx_native_view_creator creator,
                                              void* opaque) {
  Puppet<Owner::kUI, NativeViewService> native_view_service =
      view_context->GetPageView()
          ->GetServiceManager()
          ->GetService<NativeViewService>();
  native_view_service.Act([view_context, name, creator, opaque](auto& service) {
    auto& factories =
        static_cast<NativeViewServiceDesktop&>(service).view_factories;
    std::unordered_set<std::string> native_view_tags;
    for (auto& pair : factories) {
      native_view_tags.insert(pair.first);
    }
    native_view_tags.insert(name);
    view_context->SyncNativeViewTags(native_view_tags);

    factories[name] = [creator, opaque]() -> clay::NativePlatformView* {
      auto fx = reinterpret_cast<clay::NativePlatformView* (*)(void*)>(creator);
      return fx(opaque);
    };
  });
}

std::shared_ptr<NativeViewService> NativeViewService::Create() {
  return std::make_shared<NativeViewServiceDesktop>();
}

}  // namespace clay

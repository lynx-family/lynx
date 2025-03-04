// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/ios/template_assembler_recorder_darwin.h"

#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace recorder {

UITouchRecord::UITouchRecord(UITouch* touch, LynxView* view)
    : time_stamp_(touch.timestamp),
      unique_id_((int64_t)touch),
      touch_phase_(touch.phase),
      tap_count_(touch.tapCount),
      touch_type_(touch.type),
      major_radius_(touch.majorRadius),
      major_radius_tolerance_(touch.majorRadiusTolerance) {
  location_in_lynx_view_x_ = [touch locationInView:view].x;
  location_in_lynx_view_y_ = [touch locationInView:view].y;
}

UIEventRecord::UIEventRecord(UIEvent* event, LynxView* view)
    : time_stamp_(event.timestamp), type_(event.type), subtype_(event.subtype) {
  [event.allTouches enumerateObjectsUsingBlock:^(UITouch* _Nonnull obj, BOOL* _Nonnull stop) {
    if (obj != nil) {
      touchs_.push_back(UITouchRecord(obj, view));
    }
  }];
}

void TemplateAssemblerRecorderDarwin::RecordPlatformEventDarwin(UIEvent* event, LynxView* view) {
  constexpr const static char* kSendEventDarwin = "sendEventDarwin";
  constexpr const static char* kUIEvent = "UIEvent";
  constexpr const static char* kTimeStamp = "timestamp";
  constexpr const static char* kType = "type";
  constexpr const static char* kSubType = "subtype";
  constexpr const static char* kAllTouchs = "allTouches";
  constexpr const static char* kUniqueID = "uniqueID";
  constexpr const static char* kTouchPhase = "phase";
  constexpr const static char* kTapCount = "tapCount";
  constexpr const static char* kTouchType = "touchType";
  constexpr const static char* kMajorRadius = "majorRadius";
  constexpr const static char* kMajorRadiusTolerance = "majorRadiusTolerance";
  constexpr const static char* kLocationInLynxViewX = "locationInLynxViewX";
  constexpr const static char* kLocationInLynxViewY = "locationInLynxViewY";

  UIEventRecord event_record(event, view);

  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value para(rapidjson::kObjectType);

  // UIEvent
  rapidjson::Value ui_event(rapidjson::kObjectType);
  ui_event.AddMember(rapidjson::StringRef(kTimeStamp), event_record.time_stamp_, allocator);
  ui_event.AddMember(rapidjson::StringRef(kType), event_record.type_, allocator);
  ui_event.AddMember(rapidjson::StringRef(kSubType), event_record.subtype_, allocator);

  // UITouch
  rapidjson::Value ui_touchs(rapidjson::kArrayType);
  for (const auto& touch : event_record.touchs_) {
    rapidjson::Value touch_rapid_obj(rapidjson::kObjectType);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kTimeStamp), touch.time_stamp_, allocator);

    rapidjson::Value uid;
    std::string uid_str = std::to_string(touch.unique_id_);
    uid.SetString(uid_str.c_str(), (int)strlen(uid_str.c_str()), allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kUniqueID), uid, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kTouchPhase), touch.touch_phase_, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kTapCount), touch.tap_count_, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kTouchType), touch.touch_type_, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kMajorRadius), touch.major_radius_, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kMajorRadiusTolerance),
                              touch.major_radius_tolerance_, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kLocationInLynxViewX),
                              touch.location_in_lynx_view_x_, allocator);
    touch_rapid_obj.AddMember(rapidjson::StringRef(kLocationInLynxViewY),
                              touch.location_in_lynx_view_y_, allocator);

    ui_touchs.GetArray().PushBack(touch_rapid_obj, allocator);
  }

  ui_event.AddMember(rapidjson::StringRef(kAllTouchs), ui_touchs, allocator);
  para.AddMember(rapidjson::StringRef(kUIEvent), ui_event, allocator);

  id inspector = view.baseInspectorOwner;
  SEL method = NSSelectorFromString(@"getRecordID");
  IMP imp = [inspector methodForSelector:method];
  if ([inspector respondsToSelector:method]) {
    int64_t (*func)(id, SEL) = (__typeof__(func))imp;
    TestBenchBaseRecorder::GetInstance().RecordAction(kSendEventDarwin, para,
                                                      func(inspector, method));
  }
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

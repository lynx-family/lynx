// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/pointer_data_to_event.h"

#include <cstring>

#include "clay/fml/logging.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/window/pointer_data.h"

namespace clay {

namespace {

void CopyToEvent(clay::PointerEvent* dest, const clay::PointerData& raw_data) {
  if (raw_data.kind == clay::PointerData::DeviceKind::kMouse) {
    dest->device = clay::PointerEvent::DeviceType::kMouse;
  } else if (raw_data.kind == clay::PointerData::DeviceKind::kTrackpad) {
    dest->device = clay::PointerEvent::DeviceType::kTrackpad;
  } else {
    // TODO(Chenfeng Pan): We already have differentiated *mouse* and *trackpad*
    // in platform layer, which means raw_data.kind would be
    // clay::PointerData::DeviceKind::kTrackpad when using trackpad. However
    // setting PointerEvent.device to PointerEvent::DeviceType::kTrackpad need
    // to refactor reporting events in page_view.cc and consuming events in
    // gesture_manager.cc. Now we treat *trackpad* as *touch* temporarily to
    // statisfy functionality and reporting.
    dest->device = clay::PointerEvent::DeviceType::kTouch;
  }
  dest->embedder_id = raw_data.embedder_id;
  dest->timestamp = raw_data.time_stamp;
  dest->pointer_id = raw_data.pointer_identifier;
  dest->device_id = raw_data.device;
  dest->position = clay::FloatPoint(raw_data.physical_x, raw_data.physical_y);
  dest->delta =
      clay::FloatSize(raw_data.physical_delta_x, raw_data.physical_delta_y);
  // TODO(Xietong): the value for down only valid for touch. If we need to
  // support mouse, this should be updated.
  dest->down = raw_data.change == clay::PointerData::Change::kDown ||
               raw_data.change == clay::PointerData::Change::kMove;
  dest->buttons = raw_data.buttons;
  dest->pressure = raw_data.pressure;
  dest->pressure_min = raw_data.pressure_min;
  dest->pressure_max = raw_data.pressure_max;
  dest->distance = raw_data.distance;
  dest->distance_max = raw_data.distance_max;
  dest->size = raw_data.size;
  dest->radius_major = raw_data.radius_major;
  dest->radius_minor = raw_data.radius_minor;
  dest->radius_min = raw_data.radius_min;
  dest->radius_max = raw_data.radius_max;
  dest->orientation = raw_data.orientation;
  dest->tilt = raw_data.tilt;
  dest->platform_data = raw_data.platformData;
  dest->synthesized = false;
  dest->scroll_delta_x = raw_data.scroll_delta_x;
  dest->scroll_delta_y = raw_data.scroll_delta_y;
  dest->pan = clay::FloatPoint(raw_data.pan_x, raw_data.pan_y);
  dest->pan_delta = clay::FloatSize(raw_data.pan_delta_x, raw_data.pan_delta_y);
  dest->scale = raw_data.scale;
  dest->rotation = raw_data.rotation;
  dest->is_precise_scroll = static_cast<bool>(raw_data.is_precise_scroll);
  dest->source = raw_data.source;
}

}  // namespace

std::vector<clay::PointerEvent> GetEventsFromPointerDataPacket(
    const clay::PointerDataPacket* packet) {
  constexpr size_t data_size = sizeof(clay::PointerData);
  FML_DCHECK(packet && packet->data().size() % data_size == 0);

  std::vector<clay::PointerEvent> events;

  clay::PointerData raw_data;
  for (size_t i = 0; i < packet->data().size() / data_size; i++) {
    memcpy(&raw_data, packet->data().data() + data_size * i, data_size);
    if (raw_data.signal_kind == PointerData::SignalKind::kNone) {
      switch (raw_data.change) {
        case clay::PointerData::Change::kDown: {
          events.emplace_back(clay::PointerEvent::EventType::kDownEvent);
          clay::PointerEvent& event = events.back();
          CopyToEvent(&event, raw_data);
          event.down = true;
          event.distance = 0.0;
        } break;

        case clay::PointerData::Change::kMove: {
          events.emplace_back(clay::PointerEvent::EventType::kMoveEvent);
          clay::PointerEvent& event = events.back();

          CopyToEvent(&event, raw_data);
          event.down = true;
          event.distance = 0.0;
        } break;

        case clay::PointerData::Change::kUp: {
          events.emplace_back(clay::PointerEvent::EventType::kUpEvent);
          clay::PointerEvent& event = events.back();
          CopyToEvent(&event, raw_data);
          event.down = false;
        } break;

        case clay::PointerData::Change::kCancel: {
          events.emplace_back(clay::PointerEvent::EventType::kCancel);
          clay::PointerEvent& event = events.back();

          CopyToEvent(&event, raw_data);
          event.down = false;
        } break;

        case clay::PointerData::Change::kAdd:
          break;
        case clay::PointerData::Change::kHover: {
          // on iOS, a kAdd/kHover event could be synthesized.
          // Ignore it for now.
          auto& event =
              events.emplace_back(clay::PointerEvent::EventType::kHoverEvent);
          CopyToEvent(&event, raw_data);
        } break;

        case clay::PointerData::Change::kRemove: {
          auto& event =
              events.emplace_back(clay::PointerEvent::EventType::kCancel);
          CopyToEvent(&event, raw_data);
          event.position.SetX(0.f);
          event.position.SetY(0.f);
          event.down = false;
        } break;
        case clay::PointerData::Change::kPanZoomStart: {
          events.emplace_back(
              clay::PointerEvent::EventType::kPanZoomStartEvent);
          clay::PointerEvent& event = events.back();

          CopyToEvent(&event, raw_data);
          event.down = false;
        } break;
        case clay::PointerData::Change::kPanZoomUpdate: {
          events.emplace_back(
              clay::PointerEvent::EventType::kPanZoomUpdateEvent);
          clay::PointerEvent& event = events.back();

          CopyToEvent(&event, raw_data);
          event.down = false;
        } break;
        case clay::PointerData::Change::kPanZoomEnd: {
          events.emplace_back(clay::PointerEvent::EventType::kPanZoomEndEvent);
          clay::PointerEvent& event = events.back();

          CopyToEvent(&event, raw_data);
          event.down = false;
        } break;
        default:
          FML_LOG(ERROR) << "PointerData Change is not supported: "
                         << static_cast<int64_t>(raw_data.change);
          break;
      }
    } else if (raw_data.signal_kind == PointerData::SignalKind::kScroll) {
      events.emplace_back(clay::PointerEvent::EventType::kSignalEvent);
      clay::PointerEvent& event = events.back();
      event.signal_kind = clay::PointerEvent::SignalKind::kScroll;
      CopyToEvent(&event, raw_data);
    } else {
      // Ignore unknown signal kind.
    }
  }
  return events;
}

}  // namespace clay

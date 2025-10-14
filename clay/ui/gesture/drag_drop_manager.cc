// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/drag_drop_manager.h"

#include <utility>

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"

namespace clay {

namespace {
const char* DragDropEventTypeToString(ClayEventType type) {
  switch (type) {
    case ClayEventType::kClayEventTypeDragEnter:
      return "dragenter";
    case ClayEventType::kClayEventTypeDragOver:
      return "dragover";
    case ClayEventType::kClayEventTypeDragLeave:
      return "dragleave";
    case ClayEventType::kClayEventTypeDrop:
      return "drop";
    default:
      FML_DCHECK(false) << "Unknown drag drop event type: " << type;
      return "unknown";
  }
}
}  // namespace

DragDropManager::DragDropManager(PageView* page_view) : page_view_(page_view) {}

void DragDropManager::DropEnterAndHover(FloatPoint point) {
  FML_DLOG(INFO) << "DragDropManager::DropEnterAndHover";
  point = page_view_->ConvertFrom<kPixelTypePhysical>(point);
  if (page_view_) {
    int view_id = page_view_->GetViewIdForLocation(point.x(), point.y());
    if (view_id >= 0) {
      if (current_view_id_ == view_id) {
        // hover
        SendEvent(ClayEventType::kClayEventTypeDragOver, view_id,
                  clay::Value::Map());
      } else {
        // enter
        SendEvent(ClayEventType::kClayEventTypeDragEnter, view_id,
                  clay::Value::Map());
        // leave previous view
        DropLeave();
      }
    } else {
      DropLeave();
    }
    current_view_id_ = view_id;
  }
}

void DragDropManager::DropLeave() {
  FML_DLOG(INFO) << "DragDropManager::DropEnterAndHover: EXITED";
  if (current_view_id_ >= 0) {
    // leave
    SendEvent(ClayEventType::kClayEventTypeDragLeave, current_view_id_,
              clay::Value::Map());
  }
  current_view_id_ = -1;
}

void DragDropManager::DragDrop(
    FloatPoint point, std::string drag_type, std::string content_text,
    const std::list<std::unordered_map<std::string, std::string>>&
        content_files) {
  point = page_view_->ConvertFrom<kPixelTypePhysical>(point);
  if (drag_type.compare(clay::kDragTextType) == 0) {
    // Currently we do nothing when dragging text.
    return;
  } else if (drag_type.compare(clay::kDragFileType) == 0) {
    clay::Value::Map map;
    map["dropEffect"] = clay::Value("link");
    clay::Value::Array array(1);
    array[0] = clay::Value("Files");
    map["types"] = clay::Value(std::move(array));

    clay::Value::Array file_array;
    for (const auto& item : content_files) {
      clay::Value::Map file_map;
      for (const auto& iter : item) {
        auto key_str = iter.first;
        clay::Value value;
        if (key_str.compare(clay::kDragDropSizeKey) == 0) {
          int64_t file_size = static_cast<int64_t>(std::stoll(iter.second));
          value = clay::Value(file_size);
        } else {
          auto& tmp_str = iter.second;
          value = clay::Value(tmp_str);
        }
        file_map[key_str] = std::move(value);
      }
      file_array.emplace_back(std::move(file_map));
    }
    map["files"] = clay::Value(std::move(file_array));
    Drop(point, std::move(map));
  } else {
    FML_UNIMPLEMENTED();
  }
}

void DragDropManager::Drop(FloatPoint point, clay::Value::Map map) {
  FML_DLOG(INFO) << "DragDropManager::DropEnterAndHover: DROP";
  if (page_view_) {
    int view_id = page_view_->GetViewIdForLocation(point.x(), point.y());
    if (view_id >= 0) {
      SendEvent(ClayEventType::kClayEventTypeDrop, view_id, std::move(map));
    } else {
      DropLeave();
    }
  }
  current_view_id_ = -1;
}

void DragDropManager::SendEvent(ClayEventType type, int view_id,
                                clay::Value::Map map) {
  if (!page_view_) {
    return;
  }
  if (page_view_->GetEventDelegate()) {
    page_view_->GetEventDelegate()->OnDragDropEvent(
        DragDropEventTypeToString(type), view_id, std::move(map));
  }
}

}  // namespace clay

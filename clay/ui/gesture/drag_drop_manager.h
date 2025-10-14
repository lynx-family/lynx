// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_DRAG_DROP_MANAGER_H_
#define CLAY_UI_GESTURE_DRAG_DROP_MANAGER_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/public/clay.h"
#include "clay/public/value.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class PageView;

constexpr char kDragTextType[] = "drag_text";
constexpr char kDragFileType[] = "drag_file";
constexpr char kDragDropPathKey[] = "path";
constexpr char kDragDropNameKey[] = "name";
constexpr char kDragDropTypeKey[] = "type";
constexpr char kDragDropSizeKey[] = "size";
constexpr char kDragDropLastModifiedKey[] = "lastModified";

class DragDropManager {
 public:
  explicit DragDropManager(PageView* page_view);
  DragDropManager(const DragDropManager&) = delete;
  DragDropManager& operator=(const DragDropManager&) = delete;

  void DropEnterAndHover(FloatPoint point);
  void DropLeave();
  void DragDrop(FloatPoint point, std::string drag_type,
                std::string content_text,
                const std::list<std::unordered_map<std::string, std::string>>&
                    content_files);

 private:
  void Drop(FloatPoint point, clay::Value::Map map);
  void SendEvent(ClayEventType type, int view_id, clay::Value::Map map);

  PageView* page_view_ = nullptr;
  int current_view_id_ = -1;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_DRAG_DROP_MANAGER_H_

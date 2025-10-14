// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_MOUSE_CURSOR_MANAGER_H_
#define CLAY_UI_GESTURE_MOUSE_CURSOR_MANAGER_H_

#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "clay/net/resource_type.h"
#include "clay/ui/component/mouse_cursor.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/platform/cursor_types.h"

namespace clay {

class BaseView;

class MouseCursorManager {
 public:
  using ActiveCursorCallback = std::function<void(const Cursor&)>;

  explicit MouseCursorManager(ActiveCursorCallback active_cursor_callback);

  void AddCursorHolder(BaseView* holder);

  void DeleteHolder(BaseView* holder);

  // Decide which one to display or not
  void HandleHitTestResult(const std::list<fml::WeakPtr<BaseView>>& result);

  // clean cursor cache either [cursor_cache_ ] or [FlutterEngine] if it has.
  void CleanCache();

 private:
  void DisplayCursor(const Cursor& cursor);
  const Cursor* GetCursorFromCursors(const std::vector<Cursor>& vec);

  const Cursor default_cursor_ = {CursorTypes::kBasic, std::string("")};

  fml::WeakPtr<HitTestTarget> prev_hittest_target_;
  ActiveCursorCallback active_cursor_callback_;

  std::map<BaseView*, const Cursor*> cursor_cache_;
  // cache net and local resource
  std::map<std::string, RawResource> resource_cache_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_MOUSE_CURSOR_MANAGER_H_

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/mouse_cursor_manager.h"

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"

namespace clay {
MouseCursorManager::MouseCursorManager(
    ActiveCursorCallback active_cursor_callback)
    : active_cursor_callback_(active_cursor_callback) {}

void MouseCursorManager::AddCursorHolder(BaseView* holder) {
  FML_DCHECK(holder != nullptr);

  if (cursor_cache_.find(holder) != cursor_cache_.end()) {
    cursor_cache_.erase(holder);
  }

  cursor_cache_.emplace(holder, nullptr);
}

void MouseCursorManager::DeleteHolder(BaseView* holder) {
  cursor_cache_.erase(holder);
}

void MouseCursorManager::HandleHitTestResult(
    const std::list<fml::WeakPtr<BaseView>>& result) {
  for (const auto& iter : result) {
    auto* iter_base_view = static_cast<BaseView*>(iter.get());
    auto cursor_iter = cursor_cache_.find(iter_base_view);
    if (cursor_iter == cursor_cache_.end() ||
        iter_base_view->IsInternalView()) {
      continue;
    }

    if (iter == prev_hittest_target_) {
      break;
    }

    prev_hittest_target_ = iter;
    if (iter_base_view->IsInternalView()) {
      return;
    }
    if (cursor_iter->second == nullptr) {
      BaseView* view = static_cast<BaseView*>(cursor_iter->first);
      auto* mouse_cursor = view->GetMouseCursor();
      if (mouse_cursor == nullptr) {
        cursor_iter->second = &default_cursor_;
      } else {
        auto& cursor_vec = mouse_cursor->GetCursors();
        cursor_iter->second = GetCursorFromCursors(cursor_vec);
      }
    }

    DisplayCursor(*cursor_iter->second);
    return;
  }
}

const Cursor* MouseCursorManager::GetCursorFromCursors(
    const std::vector<Cursor>& vec) {
  auto iter = vec.begin();
  if (iter != vec.end()) {
    if (iter->type == CursorTypes::kNet || iter->type == CursorTypes::kFile) {
      // TODO(jiangwenlong) : handle network and file
    } else {
      return &(*iter);
    }
  }
  return &default_cursor_;
}

void MouseCursorManager::DisplayCursor(const Cursor& cursor) {
  if (cursor.type == CursorTypes::kNet || cursor.type == CursorTypes::kFile) {
    FML_DCHECK(false) << "TODO(jiangwenlong) : support net and local file";
    return;
  }

  if (active_cursor_callback_) {
    active_cursor_callback_(cursor);
  }
}

void MouseCursorManager::CleanCache() {
  // Now wd don't need this. Support it when we support local and network
  // cursor.
  FML_DCHECK(false) << "TODO(jiangwenlong) : support this";
}

}  // namespace clay

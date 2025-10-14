// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/drag_drop_service.h"

#include "clay/shell/common/engine.h"

namespace clay {
// static
std::shared_ptr<DragDropService> DragDropService::Create() {
  return std::make_shared<DragDropService>();
}

void DragDropService::OnInit(ServiceManager&, const UIServiceContext& ctx) {
  drag_drop_manager_ =
      std::make_unique<DragDropManager>(ctx.engine->GetPageView());
}

void DragDropService::OnPlatformDragLeave() {
  if (drag_drop_manager_) {
    drag_drop_manager_->DropLeave();
  }
}

void DragDropService::OnPlatformDragDrop(
    FloatPoint point, std::string drag_type, std::string content_text,
    const std::list<std::unordered_map<std::string, std::string>>&
        content_files) {
  if (drag_drop_manager_) {
    drag_drop_manager_->DragDrop(point, drag_type, content_text, content_files);
  }
}

void DragDropService::OnPlatformDragEnterAndOver(FloatPoint point) {
  if (drag_drop_manager_) {
    drag_drop_manager_->DropEnterAndHover(point);
  }
}

}  // namespace clay

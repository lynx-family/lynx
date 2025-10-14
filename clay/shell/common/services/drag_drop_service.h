// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_DRAG_DROP_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_DRAG_DROP_SERVICE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "clay/common/service/service.h"
#include "clay/common/service/service_manager.h"
#include "clay/ui/gesture/drag_drop_manager.h"

namespace clay {

class DragDropService : public Service<DragDropService, clay::Owner::kUI> {
 public:
  static std::shared_ptr<DragDropService> Create();

  void OnPlatformDragLeave();
  void OnPlatformDragDrop(
      FloatPoint point, std::string drag_type, std::string content_text,
      const std::list<std::unordered_map<std::string, std::string>>&
          content_files);
  void OnPlatformDragEnterAndOver(FloatPoint point);

 private:
  void OnInit(ServiceManager&, const UIServiceContext& ctx) override;

  std::unique_ptr<DragDropManager> drag_drop_manager_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_DRAG_DROP_SERVICE_H_

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/rasterizer_service.h"

#include "clay/common/service/service_manager.h"
#include "clay/shell/common/rasterizer.h"

namespace clay {

std::shared_ptr<RasterizerService> RasterizerService::Create() {
  return std::make_shared<RasterizerService>();
}

void RasterizerService::OnInit(clay::ServiceManager& service_manager,
                               const clay::RasterServiceContext& ctx) {
  rasterizer_ = ctx.rasterizer;
}

}  // namespace clay

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_RASTERIZER_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_RASTERIZER_SERVICE_H_

#include <memory>

#include "clay/common/service/service.h"

namespace clay {

class RasterizerService
    : public clay::Service<RasterizerService, clay::Owner::kRaster> {
 public:
  static std::shared_ptr<RasterizerService> Create();

  Rasterizer* GetRasterizer() const { return rasterizer_; }

 private:
  void OnInit(clay::ServiceManager&,
              const clay::RasterServiceContext& ctx) override;

  Rasterizer* rasterizer_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_RASTERIZER_SERVICE_H_

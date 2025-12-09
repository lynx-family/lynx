// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_TEMPLATE_DATA_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_TEMPLATE_DATA_PRIV_H_

#include <memory>

#include "core/renderer/data/template_data.h"
#include "platform/embedder/public/capi/lynx_template_data_capi.h"

struct lynx_template_data_t {
  std::shared_ptr<lynx::tasm::TemplateData> template_data = nullptr;
};

#endif  // PLATFORM_EMBEDDER_LYNX_TEMPLATE_DATA_PRIV_H_

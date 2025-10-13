// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_TEMPLATE_BUNDLE_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_TEMPLATE_BUNDLE_PRIV_H_

#include <memory>
#include <string>

#include "core/template_bundle/lynx_template_bundle.h"
#include "platform/embedder/public/capi/lynx_template_bundle_capi.h"

struct lynx_template_bundle_t {
  std::shared_ptr<lynx::tasm::LynxTemplateBundle> template_bundle;
  std::string error;
};

#endif  // PLATFORM_EMBEDDER_LYNX_TEMPLATE_BUNDLE_PRIV_H_

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspected_context_provider.h"

#include "base/include/log/logging.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"

namespace lepus_inspector {

std::shared_ptr<LepusInspectedContext>
LepusInspectedContextProvider::GetInspectedContext(
    lynx::lepus::Context* context, LepusInspectorNGImpl* inspector,
    const std::string& name) {
  auto inspected_context =
      std::make_shared<LepusNGInspectedContextImpl>(inspector, context, name);
  LOGI("lepus debug: create LepusNGInspectedContextImpl " << inspected_context);
  return inspected_context;
}

}  // namespace lepus_inspector

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_PROVIDER_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_PROVIDER_H_

#include <memory>
#include <string>

#include "core/runtime/vm/lepus/context.h"

namespace lepus_inspector {

class LepusInspectorNGImpl;
class LepusInspectedContext;

class LepusInspectedContextProvider {
 public:
  static std::shared_ptr<LepusInspectedContext> GetInspectedContext(
      lynx::lepus::Context* context, LepusInspectorNGImpl* inspector,
      const std::string& name);
};

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_PROVIDER_H_

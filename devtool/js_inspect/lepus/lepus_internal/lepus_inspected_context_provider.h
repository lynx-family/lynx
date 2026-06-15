// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_PROVIDER_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_PROVIDER_H_

#include <memory>
#include <string>

#include "core/runtime/mts_context.h"

namespace lepus_inspector {

class LepusInspectorNGImpl;
class LepusInspectedContext;

class LepusInspectedContextProvider {
 public:
  // Factory signature matches GetInspectedContext. An internal build can
  // register a classic-Lepus-capable factory to override the default
  // (LepusNG-only) implementation, without creating a build dependency from
  // the open-source tree onto internal targets.
  using Factory = std::shared_ptr<LepusInspectedContext> (*)(
      lynx::runtime::MTSContext* context, LepusInspectorNGImpl* inspector,
      const std::string& name);

  static std::shared_ptr<LepusInspectedContext> GetInspectedContext(
      lynx::runtime::MTSContext* context, LepusInspectorNGImpl* inspector,
      const std::string& name);

  // Registers an override factory. Passing nullptr restores the default
  // open-source implementation.
  static void RegisterFactory(Factory factory);
};

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_PROVIDER_H_

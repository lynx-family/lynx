// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspected_context_provider.h"

#include <atomic>

#include "base/include/log/logging.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"

namespace lepus_inspector {

namespace {
std::atomic<LepusInspectedContextProvider::Factory> g_factory{nullptr};
}  // namespace

void LepusInspectedContextProvider::RegisterFactory(Factory factory) {
  g_factory.store(factory, std::memory_order_release);
}

std::shared_ptr<LepusInspectedContext>
LepusInspectedContextProvider::GetInspectedContext(
    lynx::runtime::MTSContext* context, LepusInspectorNGImpl* inspector,
    const std::string& name) {
  auto factory = g_factory.load(std::memory_order_acquire);
  if (factory) {
    return factory(context, inspector, name);
  }
  // Default open-source implementation: LepusNG only.
  auto inspected_context =
      std::make_shared<LepusNGInspectedContextImpl>(inspector, context, name);
  LOGI("lepus debug: create LepusNGInspectedContextImpl " << inspected_context);
  return inspected_context;
}

}  // namespace lepus_inspector

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/template_entry_holder.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/runtime/bts/bts_runtime.h"

namespace lynx {

// Encoder tests have no page engine, BTS runtime or template entries.
// Supply the accessors referenced by memory queries without dead stripping.
namespace shell {
tasm::TemplateAssembler* LynxEngine::GetTasm() { return nullptr; }

base::UnsafeWeakPtr<runtime::js::Runtime> BTSRuntime::GetJSRuntimeWeak() {
  return {};
}
}  // namespace shell

namespace tasm {
std::shared_ptr<TemplateEntry> TemplateEntryHolder::FindTemplateEntry(
    const std::string& entry_name) {
  return nullptr;
}
}  // namespace tasm

}  // namespace lynx

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// OSS-specific `SerializeMTSBundle`. The rest of the converter's
// implementation lives in `lynx_template_bundle_converter_impl.cc` so the OSS
// and internal builds do not have to keep two copies of the shared logic in
// sync. See that file for the entry-point declaration.

#include "core/template_bundle/lynx_template_bundle_converter.h"

#include <memory>

#include "core/runtime/lepusng/quick_context.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

void SerializeMTSBundle(
    const std::shared_ptr<runtime::ContextBundle>& context_bundle,
    rapidjson::Document& document) {
  // serialize just support lepusng
  if (context_bundle && context_bundle->IsLepusNG()) {
    auto& allocator = document.GetAllocator();
    auto* bundle =
        static_cast<lepus::QuickContextBundle*>(context_bundle.get());
    rapidjson::Value lepus_code(rapidjson::kArrayType);
    for (const auto& element : bundle->lepus_code()) {
      lepus_code.PushBack(element, allocator);
    }
    document.AddMember("lepus_code", lepus_code, allocator);
    document.AddMember("lepus_code_len",
                       rapidjson::Value(bundle->lepusng_code_len()), allocator);
  }
}

}  // namespace tasm
}  // namespace lynx

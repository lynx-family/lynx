// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/base_binary_reader.h"

#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace lepus {
bool BaseBinaryReader::DeserializeStringSection() { return true; }

bool BaseBinaryReader::DecodeUtf8Str(base::String& result) {
  ReadStringDirectly(result);
  return true;
}

bool BaseBinaryReader::DecodeUtf8Str(std::string* result) {
  ReadStringDirectly(result);
  return true;
}

bool BaseBinaryReader::DecodeUtf8Str(lynx_value& result) {
  base::String tmp;
  ReadStringDirectly(tmp);
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(tmp);
  result.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  result.type = lynx_value_string;
  base::String::Unsafe::SetStringToEmpty(tmp);
  return true;
}

bool BaseBinaryReader::DecodeContextBundle(runtime::ContextBundle* bundle) {
  if (bundle->IsLepusNG()) {
    if (DecodeLepusNGContextBundleImpl(bundle)) {
      return true;
    }
    PrintError("Function %s, %d\n", __FUNCTION__, __LINE__);
    return false;
  }
#if !ENABLE_JUST_LEPUSNG
  if (DecodeVMContextBundleImpl(bundle)) {
    return true;
  }
#endif
  PrintError("Function: %s, %d\n", __FUNCTION__, __LINE__);
  return false;
}

}  // namespace lepus
}  // namespace lynx

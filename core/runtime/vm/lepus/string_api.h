// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_STRING_API_H_
#define CORE_RUNTIME_VM_LEPUS_STRING_API_H_

#include <string>
#include <utility>

#include "core/runtime/vm/lepus/builtin.h"

namespace lynx {
namespace lepus {
int GetRegExpFlags(const std::string& flags);
std::pair<size_t, bool> GetUnicodeFromUft8(const char* input, size_t input_len,
                                           uint16_t* output,
                                           size_t output_length);

void RegisterStringAPI(Context* ctx);
void RegisterStringPrototypeAPI(Context* ctx);
std::string GetReplaceStr(const std::string& data,
                          const std::string& need_to_replace_str,
                          const std::string& replace_to_str, int32_t position);
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_STRING_API_H_

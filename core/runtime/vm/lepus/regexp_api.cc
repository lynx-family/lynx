// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/regexp_api.h"

#include <string>
#include <utility>

#include "base/include/vector.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/regexp.h"
#include "core/runtime/vm/lepus/string_api.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/vm_context.h"

#define CAPTURE_COUNT_MAX 255
extern "C" {
#include "quickjs/include/libregexp.h"
}

namespace lynx {
namespace lepus {
static Value Test(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(context->GetParam(params_count - 1)->IsRegExp());
  auto reg_exp = context->GetParam(params_count - 1)->RegExp();

  const std::string& pattern = reg_exp->get_pattern().str();
  const std::string& flags = reg_exp->get_flags().str();

  const char* input;
  size_t input_len = 0;
  if (params_count != 1) {
    DCHECK(params_count == 2);
    const auto& input_str = context->GetParam(0)->StdString();
    input = input_str.c_str();
    input_len = input_str.length();
  } else {
    input = "undefined";
    input_len = 9;
  }

  base::InlineVector<uint16_t, 512> str_c;
  str_c.resize<false>(input_len);
  const auto [unicode_len, has_unicode] =
      GetUnicodeFromUft8(input, input_len, str_c.data(), str_c.size());

  uint8_t* bc;
  char error_msg[64];
  int len, ret;
  int re_flags = GetRegExpFlags(flags);
  bc = lre_compile(&len, error_msg, sizeof(error_msg), pattern.c_str(),
                   pattern.length(), re_flags, nullptr);
  DCHECK(bc);

  uint8_t* capture[CAPTURE_COUNT_MAX * 2];

  int shift = has_unicode ? 1 : 0;
  ret = lre_exec(capture, bc, reinterpret_cast<uint8_t*>(str_c.data()), 0,
                 static_cast<int>(unicode_len), shift, nullptr);
  // free bc
  free(bc);
  Value result = Value(static_cast<bool>(ret));
  return result;
}

void RegisterREGEXPPrototypeAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "test", &Test);
  reinterpret_cast<VMContext*>(ctx)->SetRegexpPrototype(
      Value(std::move(table)));
}
}  // namespace lepus
}  // namespace lynx

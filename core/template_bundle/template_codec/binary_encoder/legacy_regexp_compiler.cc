// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_encoder/legacy_regexp_compiler.h"

#include <cstdlib>

#include "third_party/legacy_regexp/legacy_regexp_compiler.h"
extern "C" {
#include "quickjs/include/libregexp.h"
}
#include "quickjs/include/quickjs-inner.h"
#include "quickjs/include/trace-gc.h"

namespace lynx {
namespace tasm {
namespace {
static_assert(LRE_FLAG_GLOBAL == 0x01 && LRE_FLAG_IGNORECASE == 0x02 &&
                  LRE_FLAG_MULTILINE == 0x04 && LRE_FLAG_DOTALL == 0x08 &&
                  LRE_FLAG_UTF16 == 0x10 && LRE_FLAG_STICKY == 0x20 &&
                  LRE_FLAG_NAMED_GROUPS == 0x80,
              "The template regexp format requires the established flag bits");

LEPUSValue CompileTemplateRegExp(LEPUSContext* ctx, LEPUSValueConst pattern,
                                 LEPUSValueConst flags) {
  LEPUSValue pattern_root = pattern;
  LEPUSValue flags_root = flags;
  HandleScope scope(ctx, &pattern_root, HANDLE_TYPE_LEPUS_VALUE);
  scope.PushHandle(&flags_root, HANDLE_TYPE_LEPUS_VALUE);
  const char* text = nullptr;
  scope.PushHandle(&text, HANDLE_TYPE_CSTRING);
  int re_flags = 0;
  if (!LEPUS_IsUndefined(flags_root)) {
    text = LEPUS_ToCString(ctx, flags_root);
    if (!text) return LEPUS_EXCEPTION;
    bool valid = true;
    for (const char* p = text; *p; ++p) {
      int mask = 0;
      switch (*p) {
        case 'g':
          mask = LRE_FLAG_GLOBAL;
          break;
        case 'i':
          mask = LRE_FLAG_IGNORECASE;
          break;
        case 'm':
          mask = LRE_FLAG_MULTILINE;
          break;
        case 's':
          mask = LRE_FLAG_DOTALL;
          break;
        case 'u':
          mask = LRE_FLAG_UTF16;
          break;
        case 'y':
          mask = LRE_FLAG_STICKY;
          break;
        default:
          valid = false;
          break;
      }
      if (re_flags & mask) valid = false;
      re_flags |= mask;
    }
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, text);
    text = nullptr;
    if (!valid)
      return LEPUS_ThrowSyntaxError(ctx, "invalid regular expression flags");
  }
  size_t length = 0;
  text = LEPUS_ToCStringLen2(ctx, &length, pattern_root,
                             !(re_flags & LRE_FLAG_UTF16));
  if (!text) return LEPUS_EXCEPTION;
  int bytecode_length = 0;
  char error[64];
  uint8_t* bytecode = legacy_regexp::Compile(
      &bytecode_length, error, sizeof(error), text, length, re_flags, nullptr);
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, text);
  text = nullptr;
  if (!bytecode) return LEPUS_ThrowSyntaxError(ctx, "%s", error);
  LEPUSValue result = js_new_string8_len(
      ctx, reinterpret_cast<const char*>(bytecode), bytecode_length);
  free(bytecode);
  return result;
}
}  // namespace

void InstallLegacyRegExpCompiler(LEPUSContext* ctx) {
  ctx->compile_regexp = CompileTemplateRegExp;
}

}  // namespace tasm
}  // namespace lynx

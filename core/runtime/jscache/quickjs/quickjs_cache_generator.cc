// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/quickjs/quickjs_cache_generator.h"

#include <climits>
#include <memory>
#include <string>
#include <utility>

#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {
namespace cache {

namespace {
// In Lynx Lite LOGE is defined as an empty statement, so this function might be
// unused.
[[maybe_unused]] std::string GetErrorMessage(LEPUSContext *ctx,
                                             LEPUSValue &exception_value) {
  std::string error_message;
  auto str = LEPUS_ToCString(ctx, exception_value);
  if (str) {
    error_message.append(str);
  }
  if (!LEPUS_IsGCMode(ctx)) {
    LEPUS_FreeCString(ctx, str);
  }
  return error_message;
}
}  // namespace

report_func CacheGenerator::trig_mem_info_event_ = nullptr;
QuickjsCacheGenerator::QuickjsCacheGenerator(
    std::string source_url, std::shared_ptr<const Buffer> src_buffer)
    : source_url_(std::move(source_url)), src_buffer_(std::move(src_buffer)) {}

std::shared_ptr<Buffer> QuickjsCacheGenerator::GenerateCache() {
  std::string cache;
  if (!GenerateCacheImpl(source_url_, src_buffer_, cache)) {
    return nullptr;
  }
  return std::make_shared<StringBuffer>(std::move(cache));
}

std::shared_ptr<Buffer> QuickjsCacheGenerator::GenerateCache(LEPUSContext *ctx,
                                                             LEPUSValue &func) {
  LEPUS_SetMaxStackSize(ctx, static_cast<size_t>(ULONG_MAX));
  std::string cache;
  auto ret = CompileJS(ctx, source_url_, src_buffer_, cache);
  if (LEPUS_IsException(ret)) {
    return nullptr;
  }
  func = ret;
  return std::make_shared<StringBuffer>(std::move(cache));
}

bool QuickjsCacheGenerator::GenerateCacheImpl(
    const std::string &source_url, const std::shared_ptr<const Buffer> &buffer,
    std::string &contents) {
  bool ret;
  LEPUSRuntime *rt = LEPUS_NewRuntimeWithMode(0);
  if (!rt) {
    LOGE("makeCache init quickjs runtime failed!");
    return false;
  }
  if (tasm::LynxEnv::GetInstance().IsDisableTracingGC()) {
    LEPUS_SetRuntimeInfo(rt, "Lynx_JS_RC");
  } else {
    LEPUS_SetRuntimeInfo(rt, "Lynx_JS");
  }
  if (trig_mem_info_event_) {
    RegisterGCInfoCallback(rt, trig_mem_info_event_);
  }
  LEPUSContext *ctx = LEPUS_NewContext(rt);
  if (!ctx) {
    LOGE("init quickjs context failed!");
    LEPUS_FreeRuntime(rt);
    return false;
  }
  LEPUS_SetMaxStackSize(ctx, static_cast<size_t>(ULONG_MAX));

  auto result = CompileJS(ctx, source_url, buffer, contents);
  if ((ret = !LEPUS_IsException(result)) && !LEPUS_IsGCMode(ctx))
    LEPUS_FreeValue(ctx, result);
  LEPUS_FreeContext(ctx);
  LEPUS_FreeRuntime(rt);
  return ret;
}

LEPUSValue QuickjsCacheGenerator::CompileJS(
    LEPUSContext *ctx, const std::string &source_url,
    const std::shared_ptr<const Buffer> &buffer, std::string &contents) {
  int eval_flags;
  LEPUSValue obj;

  eval_flags = LEPUS_EVAL_FLAG_COMPILE_ONLY | LEPUS_EVAL_TYPE_GLOBAL |
               (enable_strip_ ? LEPUS_EVAL_FLAG_STRIP : 0);
  obj = LEPUS_Eval(ctx, reinterpret_cast<const char *>(buffer->data()),
                   buffer->size(), source_url.c_str(), eval_flags);
  if (LEPUS_IsException(obj)) {
    LOGE("CompileJS failed:" << source_url);
    UNUSED_LOG_VARIABLE LEPUSValue exception_val = LEPUS_GetException(ctx);
    auto holder = detail::QuickjsHelper::createJSValue(ctx, exception_val);
    LOGE(GetErrorMessage(ctx, exception_val));
    return LEPUS_EXCEPTION;
  }

  HandleScope func_scope(ctx, &obj, HANDLE_TYPE_LEPUS_VALUE);
  uint8_t *out_buf;
  size_t out_buf_len;
  auto holder =
      detail::QuickjsHelper::createJSValue(ctx, LEPUS_DupValue(ctx, obj));
  out_buf = LEPUS_WriteObject(ctx, &out_buf_len, obj, LEPUS_WRITE_OBJ_BYTECODE);
  if (!out_buf) {
    LOGE("out_buf has error!");
    UNUSED_LOG_VARIABLE LEPUSValue exception_val = LEPUS_GetException(ctx);
    auto holder = detail::QuickjsHelper::createJSValue(ctx, exception_val);
    LOGE(GetErrorMessage(ctx, exception_val));
    if (!LEPUS_IsGCMode(ctx)) {
      LEPUS_FreeValue(ctx, obj);
    }
    return LEPUS_EXCEPTION;
  }
  contents.append(reinterpret_cast<char *>(out_buf), out_buf_len);
  if (!LEPUS_IsGCMode(ctx)) {
    lepus_free(ctx, out_buf);
  }
  return obj;
}

}  // namespace cache
}  // namespace piper
}  // namespace lynx

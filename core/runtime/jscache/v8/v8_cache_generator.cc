// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/v8/v8_cache_generator.h"

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/jsi/v8/v8_context_wrapper_impl.h"
#include "core/runtime/jsi/v8/v8_helper.h"
#include "core/runtime/jsi/v8/v8_isolate_wrapper_impl.h"
#include "v8.h"

namespace lynx {
namespace piper {
namespace cache {

V8CacheGenerator::V8CacheGenerator(std::string origin_url,
                                   std::shared_ptr<const Buffer> src_buffer)
    : origin_url_(std::move(origin_url)), src_buffer_(std::move(src_buffer)) {}

std::shared_ptr<Buffer> V8CacheGenerator::GenerateCache() {
  std::string cache;
  if (!GenerateCacheImpl(origin_url_, src_buffer_, cache)) {
    return nullptr;
  }
  return std::make_shared<StringBuffer>(std::move(cache));
}

bool V8CacheGenerator::GenerateCacheImpl(
    const std::string &origin_url, const std::shared_ptr<const Buffer> &buffer,
    std::string &contents) {
  auto isolate_wrapper = std::make_shared<V8IsolateInstanceImpl>();
  isolate_wrapper->InitIsolate(nullptr, false);
  auto isolate = isolate_wrapper->Isolate();
  V8ContextWrapperImpl context_wrapper(isolate_wrapper);
  context_wrapper.Init();

  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context_wrapper.getContext());
  v8::TryCatch block(isolate);

#if V8_MAJOR_VERSION >= 9
  auto origin = std::make_unique<v8::ScriptOrigin>(
      isolate, detail::V8Helper::ConvertToV8String(isolate, origin_url));
#else
  auto origin = std::make_unique<v8::ScriptOrigin>(
      detail::V8Helper::ConvertToV8String(isolate, origin_url));
#endif

  auto str = v8::String::NewFromUtf8(
      isolate, reinterpret_cast<const char *>(buffer->data()),
      v8::NewStringType::kNormal, static_cast<int>(buffer->size()));
  if (str.IsEmpty()) {
    return false;
  }
  v8::ScriptCompiler::Source source(str.ToLocalChecked(), *origin);

  v8::MaybeLocal<v8::UnboundScript> maybe_unbound_script =
      v8::ScriptCompiler::CompileUnboundScript(
          isolate, &source, v8::ScriptCompiler::kEagerCompile);
  if (maybe_unbound_script.IsEmpty()) {
    return false;
  }
  auto unbound_script = maybe_unbound_script.ToLocalChecked();

  std::unique_ptr<v8::ScriptCompiler::CachedData> cached_data(
      v8::ScriptCompiler::CreateCodeCache(unbound_script));
  if (!cached_data) {
    return false;
  }

  contents.resize(cached_data->length);
  memcpy(contents.data(), cached_data->data, cached_data->length);
  return true;
}

}  // namespace cache
}  // namespace piper
}  // namespace lynx

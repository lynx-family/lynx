// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider_src.h"

#include <memory>
#include <utility>

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider.h"
#include "core/runtime/jscache/quickjs/quickjs_cache_generator.h"

namespace lynx {
namespace piper {
namespace quickjs {

QuickjsDebugInfoProvider::QuickjsDebugInfoProvider() {
  do {
    runtime_ = LEPUS_NewRuntime();
    if (!runtime_) {
      LOGE("new runtime failed");
      break;
    }
    context_ = LEPUS_NewContext(runtime_);
    if (!context_) {
      LEPUS_FreeRuntime(runtime_);
      LOGE("new context failed");
    }
  } while (0);
  return;
}

QuickjsDebugInfoProvider::~QuickjsDebugInfoProvider() {
  if (runtime_) {
    if (!LEPUS_IsGCModeRT(runtime_)) {
      LEPUS_FreeValueRT(runtime_, top_level_func_);
    }
  }
  LEPUS_FreeContext(context_);
  LEPUS_FreeRuntime(runtime_);
  return;
}

// A packed buffer consists of two parts: header and raw bytecode.
Bytecode QuickjsBytecodeProviderSrc::PackBytecode(
    const base::Version &target_sdk_version,
    std::shared_ptr<Buffer> raw_bytecode) {
  Bytecode::HeaderV1 header(static_cast<uint32_t>(raw_bytecode->size()),
                            target_sdk_version);
  return Bytecode(std::move(header), std::move(raw_bytecode));
}

std::optional<QuickjsBytecodeProvider> QuickjsBytecodeProviderSrc::Compile(
    const base::Version &target_sdk_version, const CompileOptions &options) {
  if (source_url_.empty() || !src_ || src_->size() == 0) {
    return std::nullopt;
  }

  auto raw_bytecode = CompileJs(target_sdk_version, options);
  if (!raw_bytecode) {
    return std::nullopt;
  }

  QuickjsBytecodeProvider provider(
      PackBytecode(target_sdk_version, std::move(raw_bytecode)));
  return provider;
}

QuickjsBytecodeProviderSrc::QuickjsBytecodeProviderSrc(
    std::string source_url, std::shared_ptr<const Buffer> src)
    : source_url_(std::move(source_url)), src_(std::move(src)) {}

std::shared_ptr<Buffer> QuickjsBytecodeProviderSrc::CompileJs(
    const base::Version &target_sdk_version, const CompileOptions &options) {
  // TODO(zhenziqi) target_sdk_version not supported by primjs for now
  cache::QuickjsCacheGenerator generator(source_url_, src_);
  if (info_ && info_->context_) {
    std::shared_ptr<Buffer> ret =
        generator.GenerateCache(info_->context_, info_->top_level_func_);
    if (LEPUS_IsGCMode(info_->context_)) {
      info_->p_val_.Reset(info_->context_, info_->top_level_func_);
    }
    return ret;
  }
  generator.SetEnableStripDebugInfo(options.strip_debug_info);
  return generator.GenerateCache();
}

QuickjsDebugInfoProvider &QuickjsBytecodeProviderSrc::GenerateDebugInfo() {
  info_ = std::make_unique<QuickjsDebugInfoProvider>();
  return *info_;
}

}  // namespace quickjs
}  // namespace piper
}  // namespace lynx

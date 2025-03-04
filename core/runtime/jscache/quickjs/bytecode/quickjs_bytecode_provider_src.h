// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_QUICKJS_BYTECODE_QUICKJS_BYTECODE_PROVIDER_SRC_H_
#define CORE_RUNTIME_JSCACHE_QUICKJS_BYTECODE_QUICKJS_BYTECODE_PROVIDER_SRC_H_

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode.h"
#include "core/runtime/jsi/jsi.h"
#include "core/template_bundle/template_codec/version.h"
extern "C" {
#include "quickjs/include/quickjs.h"
}
#ifdef OS_IOS
#include "persistent-handle.h"
#else
#include "quickjs/include/persistent-handle.h"
#endif
namespace lynx {
namespace piper {
namespace quickjs {
class QuickjsBytecodeProvider;

/*
  Used to save the result of quickjs compiling js, and used to generate
  debugging information later.
*/
struct QuickjsDebugInfoProvider {
 public:
  explicit QuickjsDebugInfoProvider();
  ~QuickjsDebugInfoProvider();
  LEPUSRuntime *runtime_ = nullptr;
  LEPUSContext *context_ = nullptr;
  LEPUSValue top_level_func_ = LEPUS_UNDEFINED;
  GCPersistent p_val_;
  std::string source_;
};

// Used to generate a `QuickjsBytecodeProvider` from source. Call `Compile()` to
// get a QuickjsBytecodeProvider.
class QuickjsBytecodeProviderSrc {
 public:
  struct CompileOptions {
    // Remove debug-info from compiled quickjs bytecode.
    bool strip_debug_info = false;
  };

  QuickjsBytecodeProviderSrc(std::string source_url,
                             std::shared_ptr<const Buffer> src);

  /**
   * Compile the js source file to get a bytecode provider.
   * @param target_sdk_version The minimum version of lynx supported by the
   * compiled JS bytecode. A higher target sdk version enables more
   * optimizations.
   * @param options compile options.
   * @return a `QuickjsBytecodeProvider` representing the compiled js bytecode.
   */
  std::optional<QuickjsBytecodeProvider> Compile(
      const base::Version &target_sdk_version, const CompileOptions &options);

  QuickjsDebugInfoProvider &GenerateDebugInfo();
  std::unique_ptr<QuickjsDebugInfoProvider> GetDebugInfoProvider() {
    return std::move(info_);
  }

 private:
  std::shared_ptr<Buffer> CompileJs(const base::Version &target_sdk_version,
                                    const CompileOptions &options);

  Bytecode PackBytecode(const base::Version &target_sdk_version,
                        std::shared_ptr<Buffer> raw_bytecode);

  std::string source_url_;
  std::shared_ptr<const Buffer> src_;
  std::unique_ptr<QuickjsDebugInfoProvider> info_{nullptr};
};

}  // namespace quickjs
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_QUICKJS_BYTECODE_QUICKJS_BYTECODE_PROVIDER_SRC_H_

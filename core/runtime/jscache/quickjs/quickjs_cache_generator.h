// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_QUICKJS_QUICKJS_CACHE_GENERATOR_H_
#define CORE_RUNTIME_JSCACHE_QUICKJS_QUICKJS_CACHE_GENERATOR_H_

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/jscache/cache_generator.h"

extern "C" {
#include "quickjs/include/quickjs.h"
}

namespace lynx {
namespace piper {
namespace cache {

class QuickjsCacheGenerator : public CacheGenerator {
 public:
  QuickjsCacheGenerator(std::string source_url,
                        std::shared_ptr<const Buffer> src_buffer);

  std::shared_ptr<Buffer> GenerateCache() override;
  std::shared_ptr<Buffer> GenerateCache(LEPUSContext *, LEPUSValue &);

  // this option removes debug-info from compiled quickjs bytecode.
  void SetEnableStripDebugInfo(bool enable_strip) {
    enable_strip_ = enable_strip;
  }

 private:
  bool GenerateCacheImpl(const std::string &source_url,
                         const std::shared_ptr<const Buffer> &buffer,
                         std::string &contents);

  LEPUSValue CompileJS(LEPUSContext *ctx, const std::string &source_url,
                       const std::shared_ptr<const Buffer> &buffer,
                       std::string &contents);

  std::string source_url_;
  std::shared_ptr<const Buffer> src_buffer_;
  bool enable_strip_{false};
};

}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_QUICKJS_QUICKJS_CACHE_GENERATOR_H_

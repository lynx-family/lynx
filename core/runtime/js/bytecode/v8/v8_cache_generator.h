// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BYTECODE_V8_V8_CACHE_GENERATOR_H_
#define CORE_RUNTIME_JS_BYTECODE_V8_V8_CACHE_GENERATOR_H_

#include <memory>
#include <string>

#include "core/runtime/js/bytecode/cache_generator.h"

namespace lynx {
namespace runtime {
namespace js {
namespace cache {

class V8CacheGenerator : public CacheGenerator {
 public:
  V8CacheGenerator(const std::string &origin_url,
                   std::shared_ptr<const Buffer> src_buffer);

  std::shared_ptr<Buffer> GenerateCache() override;

 private:
  bool GenerateCacheImpl(const std::string &origin_url,
                         const std::shared_ptr<const Buffer> &buffer,
                         std::string &contents);
};

}  // namespace cache
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BYTECODE_V8_V8_CACHE_GENERATOR_H_

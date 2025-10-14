// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_CACHE_GENERATOR_H_
#define CORE_RUNTIME_JSCACHE_CACHE_GENERATOR_H_

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace cache {
class CacheGenerator {
 public:
  CacheGenerator(const std::string& source_url,
                 std::shared_ptr<const Buffer> src_buffer)
      : source_url_(source_url), src_buffer_(std::move(src_buffer)) {}
  virtual ~CacheGenerator() = default;

  virtual std::shared_ptr<Buffer> GenerateCache() = 0;

  const std::string SourceUrl() { return source_url_; }
  std::shared_ptr<const Buffer>& SrcBuffer() { return src_buffer_; }
  std::optional<std::string>& Md5() { return md5_optional_; }
  void SetMd5(std::optional<std::string> md5) {
    md5_optional_ = std::move(md5);
  }

 protected:
  std::string source_url_;
  std::shared_ptr<const Buffer> src_buffer_;
  std::optional<std::string> md5_optional_;
};
}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_CACHE_GENERATOR_H_

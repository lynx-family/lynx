// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_CACHE_GENERATOR_H_
#define CORE_RUNTIME_JSCACHE_CACHE_GENERATOR_H_

#include <memory>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace cache {
class CacheGenerator {
 public:
  virtual ~CacheGenerator() = default;

  virtual std::shared_ptr<Buffer> GenerateCache() = 0;
  static void SetReportFunction(report_func func) {
    trig_mem_info_event_ = func;
  }

 protected:
  static report_func trig_mem_info_event_;
};
}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_CACHE_GENERATOR_H_

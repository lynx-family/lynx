// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_MEASURE_CACHE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_MEASURE_CACHE_H_

#include <memory>
#include <utility>

#include "base/include/lru_cache.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_attributes.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_measure_cache_key.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

using MeasureCache =
    lynx::base::LRUCache<TextMeasureCacheKey, fml::RefPtr<ParagraphHarmony>>;

class TextMeasureCache {
 public:
  static constexpr const size_t kDefaultCap{128};
  TextMeasureCache() : measure_cache_(new MeasureCache(kDefaultCap)) {}
  explicit TextMeasureCache(size_t capability)
      : measure_cache_(new MeasureCache(capability)) {}
  void Put(TextMeasureCacheKey key, fml::RefPtr<ParagraphHarmony> paragraph) {
    measure_cache_->Put(std::move(key), paragraph);
  }
  fml::RefPtr<ParagraphHarmony> Get(const TextMeasureCacheKey& key) {
    auto* ret = measure_cache_->Get(key);
    return ret ? *ret : nullptr;
  }

 private:
  std::unique_ptr<MeasureCache> measure_cache_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_MEASURE_CACHE_H_

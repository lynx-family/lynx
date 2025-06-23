// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_MEASURE_CACHE_KEY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_MEASURE_CACHE_KEY_H_

#include <string>
#include <utility>

#include "base/include/fml/hash_combine.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/measure_mode.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_attributes.h"

namespace lynx {
namespace tasm {
namespace harmony {

struct MeasureConstraints {
  float width{0};
  MeasureMode width_mode{MeasureMode::Indefinite};
  float height{0};
  MeasureMode height_mode{MeasureMode::Indefinite};

  bool operator==(const MeasureConstraints& rhs) const {
    return std::tie(width, width_mode, height, height_mode) ==
           std::tie(rhs.width, rhs.width_mode, rhs.height, rhs.height_mode);
  }
};

struct TextMeasureCacheKey {
  TextMeasureCacheKey() = delete;

  explicit TextMeasureCacheKey(const AttributedString& string,
                               MeasureConstraints constraints)
      : attributed_string(string), measure_constraints(constraints) {}

  TextMeasureCacheKey(const TextMeasureCacheKey& key) {
    attributed_string = key.attributed_string;
    measure_constraints = key.measure_constraints;
  }

  TextMeasureCacheKey(TextMeasureCacheKey&& key) {
    attributed_string = std::move(key.attributed_string);
    measure_constraints = std::move(key.measure_constraints);
  }

  bool operator==(const TextMeasureCacheKey& other) const {
    return attributed_string == other.attributed_string &&
           measure_constraints == other.measure_constraints;
  }

  bool operator!=(const TextMeasureCacheKey& other) const {
    return !(*this == other);
  }

  AttributedString attributed_string;
  MeasureConstraints measure_constraints{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

namespace std {
template <>
struct hash<lynx::tasm::harmony::MeasureConstraints> {
  size_t operator()(
      const lynx::tasm::harmony::MeasureConstraints& constraints) const {
    auto seed = lynx::fml::HashCombine();
    lynx::fml::HashCombineSeed(seed, constraints.width, constraints.width_mode,
                               constraints.height, constraints.height_mode);
    return seed;
  }
};

template <>
struct hash<lynx::tasm::harmony::TextMeasureCacheKey> {
  size_t operator()(lynx::tasm::harmony::TextMeasureCacheKey const& key) const {
    auto seed = lynx::fml::HashCombine();
    lynx::fml::HashCombineSeed(seed, key.measure_constraints,
                               key.attributed_string);
    return seed;
  }
};

}  // namespace std
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_TEXT_MEASURE_CACHE_KEY_H_

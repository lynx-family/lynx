// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLAY_FLOW_RASTER_CACHE_KEY_H_
#define CLAY_FLOW_RASTER_CACHE_KEY_H_

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/hash_combine.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/fml/logging.h"
#include "skity/geometry/matrix.hpp"

namespace clay {

class Layer;

enum class RasterCacheKeyType { kLayer, kPicture, kLayerChildren };

class RasterCacheKeyID {
 public:
  static constexpr uint64_t kDefaultUniqueID = 0;

  RasterCacheKeyID(uint64_t unique_id, RasterCacheKeyType type)
      : unique_id_(unique_id), type_(type) {}

  RasterCacheKeyID(std::vector<RasterCacheKeyID> child_ids,
                   RasterCacheKeyType type)
      : unique_id_(kDefaultUniqueID),
        type_(type),
        child_ids_(std::move(child_ids)) {}

  uint64_t unique_id() const { return unique_id_; }

  RasterCacheKeyType type() const { return type_; }

  const std::vector<RasterCacheKeyID>& child_ids() const { return child_ids_; }

  static std::optional<std::vector<RasterCacheKeyID>> LayerChildrenIds(
      const Layer* layer);

  std::size_t GetHash() const {
    if (cached_hash_) {
      return *cached_hash_;
    }
    std::size_t seed = fml::HashCombine();
    fml::HashCombineSeed(seed, unique_id_);
    fml::HashCombineSeed(seed, type_);
    for (auto& child_id : child_ids_) {
      fml::HashCombineSeed(seed, child_id.GetHash());
    }
    cached_hash_ = seed;
    return seed;
  }

  bool operator==(const RasterCacheKeyID& other) const {
    return unique_id_ == other.unique_id_ && type_ == other.type_ &&
           GetHash() == other.GetHash() && child_ids_ == other.child_ids_;
  }

  bool operator!=(const RasterCacheKeyID& other) const {
    return !operator==(other);
  }

 private:
  const uint64_t unique_id_;
  const RasterCacheKeyType type_;
  const std::vector<RasterCacheKeyID> child_ids_;
  mutable std::optional<std::size_t> cached_hash_;
};

enum class RasterCacheKeyKind { kLayerMetrics, kPictureMetrics };

class RasterCacheKey {
 public:
  RasterCacheKey(uint64_t unique_id, RasterCacheKeyType type,
                 const skity::Matrix& ctm)
      : RasterCacheKey(RasterCacheKeyID(unique_id, type), ctm) {}

  RasterCacheKey(RasterCacheKeyID id, const skity::Matrix& ctm)
      : id_(std::move(id)), matrix_(ctm) {
    matrix_.SetTranslateX(0);
    matrix_.SetTranslateY(0);
  }

  const RasterCacheKeyID& id() const { return id_; }
  const skity::Matrix& matrix() const { return matrix_; }

  RasterCacheKeyKind kind() const {
    switch (id_.type()) {
      case RasterCacheKeyType::kPicture:
        return RasterCacheKeyKind::kPictureMetrics;
      case RasterCacheKeyType::kLayer:
      case RasterCacheKeyType::kLayerChildren:
        return RasterCacheKeyKind::kLayerMetrics;
    }
  }

  struct Hash {
    std::size_t operator()(RasterCacheKey const& key) const {
      return key.id_.GetHash();
    }
  };

  struct Equal {
    constexpr bool operator()(const RasterCacheKey& lhs,
                              const RasterCacheKey& rhs) const {
#ifdef ENABLE_RASTER_CACHE_SCALE
      if (lhs.id_ == rhs.id_ &&
          RasterCacheUtil::IsMatrixSimilarity(lhs.matrix_) &&
          RasterCacheUtil::IsMatrixSimilarity(rhs.matrix_)) {
        // The similarity matrix with small scale(<=1.25) could reuse the same
        // cache. Since the matrix is similarity, we only need to compare the
        // uniform scale.
        float l_scale = RasterCacheUtil::GetScaleFactor(lhs.matrix_);
        float r_scale = RasterCacheUtil::GetScaleFactor(rhs.matrix_);
        auto scale_rate =
            l_scale > r_scale ? (l_scale / r_scale) : (r_scale / l_scale);
        return scale_rate <= 1.25;
      }
#endif
      return lhs.id_ == rhs.id_ && lhs.matrix_ == rhs.matrix_;
    }
  };

  template <class Value>
  using Map = std::unordered_map<RasterCacheKey, Value, Hash, Equal>;

 private:
  RasterCacheKeyID id_;

  // ctm where only fractional (0-1) translations are preserved:
  //   matrix_ = ctm;
  //   matrix_[skity::Matrix::kMTransX] = SkScalarFraction(ctm.GetTranslateX());
  //   matrix_[skity::Matrix::kMTransY] = SkScalarFraction(ctm.GetTranslateY());
  skity::Matrix matrix_;
};

}  // namespace clay

#endif  // CLAY_FLOW_RASTER_CACHE_KEY_H_

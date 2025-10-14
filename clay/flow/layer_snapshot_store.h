// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYER_SNAPSHOT_STORE_H_
#define CLAY_FLOW_LAYER_SNAPSHOT_STORE_H_

#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/time/time_delta.h"
#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

namespace clay {

/// Container for snapshot data pertaining to a given layer. A layer is
/// identified by it's unique id.
class LayerSnapshotData {
 public:
  LayerSnapshotData(int64_t layer_unique_id, const fml::TimeDelta& duration,
                    const clay::GrDataPtr& snapshot, const skity::Rect& bounds);

  ~LayerSnapshotData() = default;

  int64_t GetLayerUniqueId() const { return layer_unique_id_; }

  fml::TimeDelta GetDuration() const { return duration_; }

  clay::GrDataPtr GetSnapshot() const { return snapshot_; }

  skity::Rect GetBounds() const { return bounds_; }

 private:
  const int64_t layer_unique_id_;
  const fml::TimeDelta duration_;
  const clay::GrDataPtr snapshot_;
  const skity::Rect bounds_;
};

/// Collects snapshots of layers during frame rasterization.
class LayerSnapshotStore {
 public:
  typedef std::vector<LayerSnapshotData> Snapshots;

  LayerSnapshotStore() = default;

  ~LayerSnapshotStore() = default;

  /// Clears all the stored snapshots.
  void Clear();

  /// Adds snapshots for a given layer. `duration` marks the time taken to
  /// rasterize this one layer.
  void Add(const LayerSnapshotData& data);

  // Returns the number of snapshots collected.
  size_t Size() const { return layer_snapshots_.size(); }

  // make this class iterable
  Snapshots::iterator begin() { return layer_snapshots_.begin(); }
  Snapshots::iterator end() { return layer_snapshots_.end(); }

 private:
  Snapshots layer_snapshots_;

  BASE_DISALLOW_COPY_AND_ASSIGN(LayerSnapshotStore);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYER_SNAPSHOT_STORE_H_

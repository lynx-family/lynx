// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layer_snapshot_store.h"

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"

namespace clay {

LayerSnapshotData::LayerSnapshotData(int64_t layer_unique_id,
                                     const fml::TimeDelta& duration,
                                     const clay::GrDataPtr& snapshot,
                                     const skity::Rect& bounds)
    : layer_unique_id_(layer_unique_id),
      duration_(duration),
      snapshot_(snapshot),
      bounds_(bounds) {}

void LayerSnapshotStore::Clear() { layer_snapshots_.clear(); }

void LayerSnapshotStore::Add(const LayerSnapshotData& data) {
  layer_snapshots_.push_back(data);
}

}  // namespace clay

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_VIEW_SLICER_H_
#define CLAY_FLOW_VIEW_SLICER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "clay/flow/embedded_views.h"

namespace clay {

/// @brief Compute the required overlay layers and clip the view slices
///        according to the size and position of the platform views.
std::unordered_map<int64_t, skity::Rect> SliceViews(
    clay::GrCanvas* background_canvas,
    const std::vector<int64_t>& composition_order,
    const std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>>&
        slices,
    const std::unordered_map<int64_t, skity::Rect>& view_rects);

}  // namespace clay

#endif  // CLAY_FLOW_VIEW_SLICER_H_

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_BINDING_NAPI_CANVAS_ROUND_RECT_RADII_H_
#define THIRD_PARTY_BINDING_NAPI_CANVAS_ROUND_RECT_RADII_H_

#include "third_party/binding/napi/native_value_traits.h"

namespace lynx::binding {

struct IDLCanvasRoundRectRadii {};

// Converts the CanvasPath radii union to consecutive (x, y) double pairs.
// Length, sign and finite-value validation belong to the path operation.
template <>
struct NativeValueTraits<IDLCanvasRoundRectRadii> {
  static std::vector<double> NativeValue(const Napi::CallbackInfo& info,
                                         int32_t index);
};

}  // namespace lynx::binding

#endif  // THIRD_PARTY_BINDING_NAPI_CANVAS_ROUND_RECT_RADII_H_

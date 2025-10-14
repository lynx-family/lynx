// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_WINDOW_POINTER_DATA_HELPER_H_
#define CLAY_UI_WINDOW_POINTER_DATA_HELPER_H_

#include "clay/public/clay.h"
#include "clay/ui/window/pointer_data.h"

namespace clay {
class PointerDataHelper {
 public:
  static PointerData::Change ToPointerDataChange(ClayPointerPhase phase);
  static PointerData::DeviceKind ToPointerDataKind(
      ClayPointerDeviceKind device_kind);
  static PointerData::SignalKind ToPointerDataSignalKind(
      ClayPointerSignalKind kind);
  static int64_t PointerDataButtonsForLegacyEvent(PointerData::Change change);
};
}  // namespace clay

#endif  // CLAY_UI_WINDOW_POINTER_DATA_HELPER_H_

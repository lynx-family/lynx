// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_WINDOW_KEY_DATA_HELPER_H_
#define CLAY_UI_WINDOW_KEY_DATA_HELPER_H_

#include "clay/public/clay.h"
#include "clay/ui/window/key_data.h"

namespace clay {
class KeyDataHelper {
 public:
  static KeyEventActionType MapKeyEventType(ClayKeyEventType event_kind);
};
}  // namespace clay

#endif  // CLAY_UI_WINDOW_KEY_DATA_HELPER_H_

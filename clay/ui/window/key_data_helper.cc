// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/window/key_data_helper.h"

namespace clay {

KeyEventActionType KeyDataHelper::MapKeyEventType(ClayKeyEventType event_kind) {
  switch (event_kind) {
    case kClayKeyEventTypeUp:
      return KeyEventActionType::kUp;
    case kClayKeyEventTypeDown:
      return KeyEventActionType::kDown;
    case kClayKeyEventTypeRepeat:
      return KeyEventActionType::kRepeat;
  }
  return KeyEventActionType::kUp;
}
}  // namespace clay

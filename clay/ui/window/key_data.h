// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_WINDOW_KEY_DATA_H_
#define CLAY_UI_WINDOW_KEY_DATA_H_

#include <cstdint>

namespace clay {

static constexpr int kKeyDataFieldCount = 5;
static constexpr int kBytesPerKeyField = sizeof(int64_t);

// The change of the key event, used by KeyData.
enum class KeyEventActionType : int64_t {
  kDown = 0,
  kUp,
  kRepeat,
};

// The fixed-length sections of a KeyDataPacket.
//
// KeyData does not contain `character`, for variable-length data are stored in
// a different way in KeyDataPacket.
struct alignas(8) KeyData {
  // Timestamp in microseconds from an arbitrary and consistent start point
  uint64_t timestamp;
  KeyEventActionType type;
  uint64_t physical;
  uint64_t logical;
  // True if the event does not correspond to a native event.
  //
  // The value is 1 for true, and 0 for false.
  uint64_t synthesized;

  // Sets all contents of `Keydata` to 0.
  void Clear();
};

}  // namespace clay

#endif  // CLAY_UI_WINDOW_KEY_DATA_H_

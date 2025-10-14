// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/window/key_data.h"

#include <cstring>

namespace clay {

static_assert(sizeof(KeyData) == kBytesPerKeyField * kKeyDataFieldCount,
              "KeyData has the wrong size");

void KeyData::Clear() { memset(this, 0, sizeof(KeyData)); }

}  // namespace clay

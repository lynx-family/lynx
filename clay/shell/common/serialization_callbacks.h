// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERIALIZATION_CALLBACKS_H_
#define CLAY_SHELL_COMMON_SERIALIZATION_CALLBACKS_H_

#include "clay/gfx/rendering_backend.h"

namespace clay {

#ifndef ENABLE_SKITY
sk_sp<SkData> SerializeTypefaceWithoutData(SkTypeface* typeface, void* ctx);
sk_sp<SkData> SerializeTypefaceWithData(SkTypeface* typeface, void* ctx);
sk_sp<SkTypeface> DeserializeTypefaceWithoutData(const void* data,
                                                 size_t length, void* ctx);
#endif  // ENABLE_SKITY
}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERIALIZATION_CALLBACKS_H_

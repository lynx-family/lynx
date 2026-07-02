// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_DISPLAY_LIST_ITEM_SERIALIZER_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_DISPLAY_LIST_ITEM_SERIALIZER_H_

#include <cstddef>
#include <cstdint>

#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"

namespace lynx::tasm {

constexpr size_t kAndroidDisplayListItemBufferHeaderSize = sizeof(int32_t);
// TODO(renzhongyue): Need a better way to automatically gen the DisplayListItem
// size and offset of each filed. Current implementation can work because all
// fields are 32bits long. If we want to introduce long or bool type, the size
// and offset calculation will be more complex.
constexpr int32_t kSerializedAndroidDisplayListItemSize = 56;

void SerializeDisplayListItemsForAndroid(const DisplayListItem* items,
                                         size_t item_count,
                                         base::Vector<uint8_t>& buffer);

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_DISPLAY_LIST_ITEM_SERIALIZER_H_

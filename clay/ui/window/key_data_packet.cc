// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/window/key_data_packet.h"

#include <cstring>

#include "clay/fml/logging.h"

namespace clay {

KeyDataPacket::KeyDataPacket(const KeyData& event, const char* character) {
  size_t char_size = character == nullptr ? 0 : strlen(character);
  uint64_t char_size_64 = char_size;
  data_.resize(sizeof(uint64_t) + sizeof(KeyData) + char_size);
  memcpy(CharacterSizeStart(), &char_size_64, sizeof(char_size));
  memcpy(KeyDataStart(), &event, sizeof(KeyData));
  if (character != nullptr) {
    memcpy(CharacterStart(), character, char_size);
  }
}

KeyDataPacket::~KeyDataPacket() = default;

}  // namespace clay

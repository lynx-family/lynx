// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_WINDOW_KEY_DATA_PACKET_H_
#define CLAY_UI_WINDOW_KEY_DATA_PACKET_H_

#include <functional>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/ui/window/key_data.h"

namespace clay {

// A byte stream representing a key event.
class KeyDataPacket {
 public:
  // Build the key data packet by providing information.
  //
  // The `character` is a nullable C-string that ends with a '\0'.
  KeyDataPacket(const KeyData& event, const char* character);
  ~KeyDataPacket();

  // Prevent copying.
  KeyDataPacket(KeyDataPacket const&) = delete;
  KeyDataPacket& operator=(KeyDataPacket const&) = delete;

  const std::vector<uint8_t>& data() const { return data_; }

 private:
  // Packet structure:
  // | CharDataSize |     (1 field)
  // |   Key Data   |     (kKeyDataFieldCount fields)
  // |   CharData   |     (CharDataSize bits)

  uint8_t* CharacterSizeStart() { return data_.data(); }
  uint8_t* KeyDataStart() { return CharacterSizeStart() + sizeof(uint64_t); }
  uint8_t* CharacterStart() { return KeyDataStart() + sizeof(KeyData); }

  std::vector<uint8_t> data_;
};

}  // namespace clay

#endif  // CLAY_UI_WINDOW_KEY_DATA_PACKET_H_

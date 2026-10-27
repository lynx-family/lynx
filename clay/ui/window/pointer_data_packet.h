// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_WINDOW_POINTER_DATA_PACKET_H_
#define CLAY_UI_WINDOW_POINTER_DATA_PACKET_H_

#include <cstring>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/ui/window/pointer_data.h"

namespace clay {

class PointerDataPacket {
 public:
  explicit PointerDataPacket(size_t count);
  PointerDataPacket(uint8_t* data, size_t num_bytes);
  ~PointerDataPacket();

  void SetPointerData(size_t i, const PointerData& data);
  PointerData GetPointerData(size_t i) const;
  size_t GetLength() const;
  const std::vector<uint8_t>& data() const { return data_; }

 private:
  std::vector<uint8_t> data_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PointerDataPacket);
};

}  // namespace clay

#endif  // CLAY_UI_WINDOW_POINTER_DATA_PACKET_H_

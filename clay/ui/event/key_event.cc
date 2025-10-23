// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/event/key_event.h"

#include <sstream>

namespace clay {
namespace {
const char* c_key_event_type_string[] = {
    "KeyDown",
    "KeyUp",
    "KeyRepeat",
};
}  // namespace

std::string KeyEvent::ToString() const {
  if (type_ < KeyEventType::kDown || type_ > KeyEventType::kRepeat) {
    return "InvalidKeyEvent";
  }
  std::stringstream ss;
  ss << "KeyEvent{" << c_key_event_type_string[static_cast<int>(type_)]
     << ",Time=" << timestamp_ << std::hex << ",PhysicalKey=0x"
     << static_cast<uint64_t>(physical_) << ",LogicalKey=0x"
     << static_cast<uint64_t>(logical_) << ",Character=" << character_ << "} ";
  return ss.str();
}

}  // namespace clay

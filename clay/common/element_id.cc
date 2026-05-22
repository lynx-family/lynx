// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/element_id.h"

#include <sstream>

namespace clay {

uint64_t ElementId::NextUniqueID() {
  static std::atomic<uint64_t> next_id(1);
  uint64_t id;
  do {
    id = next_id.fetch_add(1);
  } while (id == 0);  // 0 is reserved for an invalid id.
  return id;
}

ElementId::ElementId(int view_id, uint64_t unique_id)
    : view_id_(view_id), unique_id_(unique_id) {}

std::string ElementId::ToString() const {
  std::stringstream ss;
  ss << "ElementId: view_id=" << view_id_ << " unique_id=" << unique_id_;
  return ss.str();
}

}  // namespace clay

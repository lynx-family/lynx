// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_ELEMENT_ID_H_
#define CLAY_COMMON_ELEMENT_ID_H_

#include <string>

namespace clay {

class ElementId {
 public:
  explicit ElementId(int view_id, uint64_t unique_id = NextUniqueID());

  static uint64_t NextUniqueID();

  bool operator==(const ElementId& o) const {
    return view_id_ == o.view_id_ && unique_id_ == o.unique_id_;
  }
  bool operator!=(const ElementId& o) const { return !(*this == o); }

  void UpdateViewId(int id) { view_id_ = id; }

  int view_id() const { return view_id_; }
  uint64_t unique_id() const { return unique_id_; }

  std::string ToString() const;

 private:
  int view_id_;
  uint64_t unique_id_;
};

}  // namespace clay

#endif  // CLAY_COMMON_ELEMENT_ID_H_

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_RECYCLABLE_H_
#define CLAY_COMMON_RECYCLABLE_H_

namespace clay {
class Recyclable {
 public:
  virtual ~Recyclable() = default;
  virtual void CleanForRecycle() {}
  virtual void PrepareForRecycle() {}
};

}  // namespace clay

#endif  // CLAY_COMMON_RECYCLABLE_H_

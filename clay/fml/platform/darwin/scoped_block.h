// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_DARWIN_SCOPED_BLOCK_H_
#define CLAY_FML_PLATFORM_DARWIN_SCOPED_BLOCK_H_

#include <utility>

#include "base/include/compiler_specific.h"

#if !__has_feature(objc_arc)
#error "ARC must be enabled"
#endif

namespace fml {

template <typename B>
class ScopedBlock {
 public:
  explicit ScopedBlock(B block = nullptr) : block_(block) {}

  ScopedBlock(const ScopedBlock<B>& that) : block_(that.block_) {}

  ~ScopedBlock() {}

  ScopedBlock& operator=(const ScopedBlock<B>& that) {
    reset(that.get());
    return *this;
  }

  void reset(B block = nullptr) { block_ = block; }

  bool operator==(B that) const { return block_ == that; }

  bool operator!=(B that) const { return block_ != that; }

  // NOLINTNEXTLINE
  operator B() const { return block_; }

  B get() const { return block_; }

  void swap(ScopedBlock& that) {
    B temp = that.block_;
    that.block_ = block_;
    block_ = temp;
  }

  [[nodiscard]] B release() {
    B temp = block_;
    block_ = nullptr;
    return temp;
  }

 private:
  B block_;
};

}  // namespace fml

#endif  // CLAY_FML_PLATFORM_DARWIN_SCOPED_BLOCK_H_

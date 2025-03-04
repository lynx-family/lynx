// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace lepus {

void CArray::ReleaseSelf() const { delete this; }

}  // namespace lepus
}  // namespace lynx

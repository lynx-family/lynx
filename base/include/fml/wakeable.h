// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_WAKEABLE_H_
#define BASE_INCLUDE_FML_WAKEABLE_H_

#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace fml {

/// Interface over the ability to \p WakeUp a \p fml::MessageLoopImpl.
/// \see fml::MessageLoopTaskQueues
class Wakeable {
 public:
  virtual ~Wakeable() {}

  virtual void WakeUp(fml::TimePoint time_point) = 0;
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::Wakeable;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_WAKEABLE_H_

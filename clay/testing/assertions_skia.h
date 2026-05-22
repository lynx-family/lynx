// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_ASSERTIONS_SKIA_H_
#define TESTING_ASSERTIONS_SKIA_H_

#include <ostream>

#include "skity/geometry/rect.hpp"
#include "third_party/skia/include/core/SkClipOp.h"
#include "third_party/skia/include/core/SkM44.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPoint3.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"

namespace clay {
namespace testing {

extern std::ostream& operator<<(std::ostream& os, const SkClipOp& o);
extern std::ostream& operator<<(std::ostream& os, const SkMatrix& m);
extern std::ostream& operator<<(std::ostream& os, const SkM44& m);
extern std::ostream& operator<<(std::ostream& os, const SkVector3& v);
extern std::ostream& operator<<(std::ostream& os, const SkIRect& r);
extern std::ostream& operator<<(std::ostream& os, const SkRect& r);
extern std::ostream& operator<<(std::ostream& os, const SkRRect& r);
extern std::ostream& operator<<(std::ostream& os, const SkPath& r);
extern std::ostream& operator<<(std::ostream& os, const SkPoint& r);
extern std::ostream& operator<<(std::ostream& os, const SkISize& size);
extern std::ostream& operator<<(std::ostream& os, const SkColor4f& r);
extern std::ostream& operator<<(std::ostream& os, const SkPaint& r);
extern std::ostream& operator<<(std::ostream& os, const SkSamplingOptions& s);

extern std::ostream& operator<<(std::ostream& os, const skity::Rect& r);

}  // namespace testing
}  // namespace clay

#endif  // TESTING_ASSERTIONS_SKIA_H_

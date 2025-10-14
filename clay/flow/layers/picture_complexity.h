// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_H_
#define CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_H_

#include "clay/gfx/rendering_backend.h"

namespace clay {

class PictureComplexityCalculator {
 public:
  static PictureComplexityCalculator* GetForSoftware();

  static PictureComplexityCalculator* GetForBackend(
      clay::GrGpuBackendType backend);

  virtual ~PictureComplexityCalculator() = default;

  // Returns a calculated complexity score for a given DisplayList object
#ifndef ENABLE_SKITY
  virtual unsigned int Compute(SkPicture* picture) = 0;
#else
  virtual unsigned int Compute(skity::DisplayList* picture) = 0;
#endif  // ENABLE_SKITY

  // Returns whether a given complexity score meets the threshold for
  // cachability for this particular ComplexityCalculator
  virtual bool ShouldBeCached(unsigned int complexity_score) = 0;

  // Sets a ceiling for the complexity score being calculated. By default
  // this is the largest number representable by an unsigned int.
  //
  // This setting has no effect on non-accumulator based scorers such as
  // the Naive calculator.
  virtual void SetComplexityCeiling(unsigned int ceiling) = 0;
};

class PictureNaiveComplexityCalculator : public PictureComplexityCalculator {
 public:
  static PictureComplexityCalculator* GetInstance();

#ifndef ENABLE_SKITY
  unsigned int Compute(SkPicture* picture) override {
    return picture->approximateOpCount(true);
  }
#else
  unsigned int Compute(skity::DisplayList* picture) override {
    return picture->OpCount();
  }
#endif  // ENABLE_SKITY

  bool ShouldBeCached(unsigned int complexity_score) override {
#ifndef ENABLE_SKITY
    return complexity_score > 5u;
#else
    // TODO(feiyue.1998): Maybe there is a better threshold than 200.
    // Now it is set 200 so that simple displaylist will not be cached.
    return complexity_score > 200u;
#endif  // ENABLE_SKITY
  }

  void SetComplexityCeiling(unsigned int ceiling) override {}

 private:
  PictureNaiveComplexityCalculator() {}
  static PictureNaiveComplexityCalculator* instance_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_H_

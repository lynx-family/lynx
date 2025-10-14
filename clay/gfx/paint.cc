// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint.h"

namespace clay {

Paint::Paint()
    : blendMode_(static_cast<unsigned>(clay::BlendMode::kDefaultMode)),
      drawStyle_(static_cast<unsigned>(DrawStyle::kDefaultStyle)),
      strokeCap_(static_cast<unsigned>(StrokeCap::kDefaultCap)),
      strokeJoin_(static_cast<unsigned>(StrokeJoin::kDefaultJoin)),
      isAntiAlias_(false),
      isDither_(false),
      isInvertColors_(false),
      strokeWidth_(kDefaultWidth),
      strokeMiter_(kDefaultMiter) {}

bool Paint::operator==(Paint const& other) const {
  return blendMode_ == other.blendMode_ &&            //
         drawStyle_ == other.drawStyle_ &&            //
         strokeCap_ == other.strokeCap_ &&            //
         strokeJoin_ == other.strokeJoin_ &&          //
         isAntiAlias_ == other.isAntiAlias_ &&        //
         isDither_ == other.isDither_ &&              //
         isInvertColors_ == other.isInvertColors_ &&  //
         color_ == other.color_ &&                    //
         strokeWidth_ == other.strokeWidth_ &&        //
         strokeMiter_ == other.strokeMiter_ &&        //
         Equals(colorSource_, other.colorSource_) &&  //
         Equals(colorFilter_, other.colorFilter_) &&  //
         Equals(imageFilter_, other.imageFilter_) &&  //
         Equals(maskFilter_, other.maskFilter_);
}

}  // namespace clay

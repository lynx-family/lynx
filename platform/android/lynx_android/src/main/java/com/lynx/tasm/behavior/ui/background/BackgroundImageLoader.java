// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.content.Context;

public interface BackgroundImageLoader {
  BackgroundLayerDrawable loadImage(Context ctx, String url);
}

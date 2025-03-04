// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image.model;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public interface LynxImageFetcher {
  void loadImage(@NonNull ImageRequestInfo imageRequestInfo,
      @NonNull ImageLoadListener loadListener, @Nullable AnimationListener animationListener,
      Context context);

  void releaseImage(@NonNull ImageRequestInfo imageRequestInfo);
}

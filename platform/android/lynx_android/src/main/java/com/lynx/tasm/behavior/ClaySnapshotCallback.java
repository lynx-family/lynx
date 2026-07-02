// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.graphics.Bitmap;
import androidx.annotation.NonNull;
import androidx.annotation.UiThread;

public interface ClaySnapshotCallback {
  @UiThread void onSuccess(@NonNull Bitmap bitmap);

  @UiThread void onError();
}

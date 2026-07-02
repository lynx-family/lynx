// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;
import com.lynx.tasm.behavior.ui.LynxBaseUI;

public interface ClaySnapshotProvider {
  @UiThread
  void requestRasterSnapshot(@NonNull LynxBaseUI ui, @NonNull ClaySnapshotCallback callback);
}

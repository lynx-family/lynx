// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.behavior.ui.frame.LynxFrameView;

/**
 * Interface for providing LynxFrameView instances.
 * When a custom provider is set via LynxContext.setLynxFrameViewProvider,
 * it will be used to create LynxFrameView instances instead of the default constructor.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public interface LynxFrameViewProvider {
  /**
   * Gets a LynxFrameView instance.
   * @param context The LynxContext to associate with the LynxFrameView
   * @return A LynxFrameView instance, or null to fall back to default creation
   */
  @Nullable LynxFrameView getLynxFrameView(@NonNull LynxContext context);
}

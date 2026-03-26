// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.graphics.drawable.Drawable;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.ShadowNode;

public interface MarkdownResourceContext {
  boolean isHostDestroyed();

  @Nullable LynxContext getLynxContext();

  @Nullable Drawable.Callback getDrawableCallback();

  @Nullable MeasureContext getMeasureContext();

  @Nullable AlignContext getAlignContext();

  int getChildCount();

  @Nullable ShadowNode getChildAt(int index);

  void onResourceLoaded();

  void onImageLoadError(String source, @Nullable Throwable throwable);
}

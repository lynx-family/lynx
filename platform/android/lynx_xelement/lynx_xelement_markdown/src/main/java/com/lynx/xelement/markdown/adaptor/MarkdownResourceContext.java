// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.ShadowNode;

public interface MarkdownResourceContext {
  @Nullable LynxContext getLynxContext();

  @Nullable MeasureContext getMeasureContext();

  @Nullable AlignContext getAlignContext();

  void setContentOffset(int left, int top);

  int getContentLeftOffset();

  int getContentTopOffset();

  int getChildCount();

  @Nullable ShadowNode getChildAt(int index);

  void onFontLoaded(String family, int weight, int style);

  void onImageLoaded(String url);

  void onImageLoadError(String source, @Nullable Throwable throwable);
}

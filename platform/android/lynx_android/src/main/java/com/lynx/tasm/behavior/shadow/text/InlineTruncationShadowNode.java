// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import java.util.List;
public class InlineTruncationShadowNode extends BaseTextShadowNode {
  public static final String TAG_NAME = "inline-truncation";

  @Override
  protected void generateStyleSpan(SpannableStringBuilder sb, List<SetSpanOperation> ops) {
    super.generateStyleSpan(sb, ops);

    if (needGenerateEventTargetSpan()) {
      sb.setSpan(toEventTargetSpan(), 0, sb.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
    }
  }

  @Override
  public boolean isVirtual() {
    return true;
  }
}

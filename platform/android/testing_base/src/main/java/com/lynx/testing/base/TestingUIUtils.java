// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testing.base;
import android.graphics.Rect;
import com.lynx.tasm.behavior.LynxUIOwner;

public class TestingUIUtils {
  public static void insertChild(
      final LynxUIOwner lynxUIOwner, final int tag, final int childTag, final int index) {
    lynxUIOwner.insert(tag, childTag, index);
  }

  public static void updateLayout(final LynxUIOwner lynxUIOwner, final int tag, final Rect rect) {
    updateLayout(lynxUIOwner, tag, rect, new Rect(), new Rect(), new Rect());
  }

  public static void updateLayout(final LynxUIOwner lynxUIOwner, final int tag, final Rect rect,
      final Rect paddingRect, final Rect marginRect, final Rect borderRect) {
    lynxUIOwner.updateLayout(tag, rect.left, rect.top, rect.right - rect.left,
        rect.bottom - rect.top, paddingRect.left, paddingRect.top, paddingRect.right,
        paddingRect.bottom, marginRect.left, marginRect.top, marginRect.right, marginRect.bottom,
        borderRect.left, borderRect.top, borderRect.right, borderRect.bottom, rect, null,
        rect.top - rect.bottom, 0);
  }
}

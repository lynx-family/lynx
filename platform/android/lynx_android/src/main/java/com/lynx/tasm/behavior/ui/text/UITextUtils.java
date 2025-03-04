// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.text;

import android.graphics.PointF;
import android.text.Layout;
import android.text.Spanned;
import android.view.View;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef.InlineViewEventSpan;
import com.lynx.tasm.behavior.shadow.text.EventTargetSpan;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import java.util.Set;

public class UITextUtils {
  public static Spanned getSpanned(AndroidText view) {
    if (view == null) {
      return null;
    }
    if (view.getText() instanceof Spanned) {
      return (Spanned) (view.getText());
    } else {
      return null;
    }
  }

  public static Spanned getSpanned(Layout layout) {
    if (layout == null) {
      return null;
    }
    if (layout.getText() instanceof Spanned) {
      return (Spanned) (layout.getText());
    } else {
      return null;
    }
  }

  public static EventTarget hitTest(LynxBaseUI ui, float x, float y, EventTarget parent,
      Layout layout, Spanned spanned, PointF textTranslateOffset, boolean ignoreUserInteraction) {
    if (layout == null || x > layout.getWidth() || y > layout.getHeight()) {
      return parent;
    }
    int line = layout.getLineForVertical((int) y);
    int dir = layout.getParagraphDirection(line);
    float originX = x, originY = y;

    x -= textTranslateOffset.x;
    y -= textTranslateOffset.y;

    if (y > layout.getLineBottom(line) || y < layout.getLineTop(line)
        || x < layout.getLineLeft(line) || x > layout.getLineRight(line)) {
      // after x adjust, the position is outside at current line-bounds
      return parent;
    }

    int off = layout.getOffsetForHorizontal(line, x);
    float offLeft = layout.getPrimaryHorizontal(off);

    if (dir == Layout.DIR_LEFT_TO_RIGHT) {
      // LTR mode
      // x position is in left means index is smaller
      if (x < offLeft) {
        off -= 1;
      }
    } else {
      // RTL mode
      // x position in right means index is smaller
      if (x >= offLeft) {
        off -= 1; // rtl, from left to right, the cursor index becomes smaller
      }
    }

    EventTargetSpan[] targets =
        spanned != null ? spanned.getSpans(off, off, EventTargetSpan.class) : null;

    if (targets == null || targets.length == 0) {
      return parent;
    }

    EventTarget target = parent;
    int prestart = 0;
    int preend = spanned.length();
    for (EventTargetSpan eventTargetSpan : targets) {
      eventTargetSpan.setParent(parent);
      int start = spanned.getSpanStart(eventTargetSpan);
      int end = spanned.getSpanEnd(eventTargetSpan);
      // if language is ltr, the off is between [start, end)
      // if language is rtl, the off is between (start, end]
      if (off >= start && off <= end && start >= prestart && end <= preend) {
        target = eventTargetSpan;
        eventTargetSpan.setParent(parent);
      }
      prestart = start;
      preend = end;
    }

    // If target's type is InlineViewEventSpan, it is an inline view.
    // Try to find the eventTarget from inline children.
    if (target instanceof InlineViewEventSpan) {
      for (LynxBaseUI childUI : ui.getChildren()) {
        if (childUI.getSign() == target.getSign()) {
          // input position relative to inline view
          return childUI.hitTest(originX - childUI.getOriginLeft(),
              originY - childUI.getOriginTop(), ignoreUserInteraction);
        }
      }
    }

    return target;
  }

  public static void HandleInlineViewTruncated(TextUpdateBundle bundle, LynxBaseUI textUI) {
    final Set viewTruncatedSet = bundle.getViewTruncatedSet();
    // handle text ellipsis
    for (LynxBaseUI baseChild : textUI.getChildren()) {
      // Inline view does not support flatten!
      if (!(baseChild instanceof LynxUI)) {
        continue;
      }
      LynxUI child = (LynxUI) baseChild;

      boolean isTruncated = viewTruncatedSet != null && viewTruncatedSet.contains(child.getSign());
      // FIXME(liting.src): inlineview in text does not support change visibility.
      if (isTruncated && child.getVisibility()) {
        child.setVisibilityForView(View.INVISIBLE);
      } else if (!isTruncated && !child.getVisibility()) {
        child.setVisibilityForView(View.VISIBLE);
      }
    }
  }
}

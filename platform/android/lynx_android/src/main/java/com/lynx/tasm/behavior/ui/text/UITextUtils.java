// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.text;

import static com.lynx.tasm.event.LynxTouchEvent.EVENT_CLICK;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_LONG_PRESS;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TAP;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_CANCEL;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_END;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_MOVE;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TOUCH_START;

import android.graphics.PointF;
import android.text.Layout;
import android.text.Spanned;
import android.view.View;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef.InlineViewEventSpan;
import com.lynx.tasm.behavior.shadow.text.EventTargetSpan;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.service.ILynxTextService.Page;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class UITextUtils {
  private static final int TEXT_EVENT_TARGET_TAP = 1 << 0;
  private static final int TEXT_EVENT_TARGET_CLICK = 1 << 1;
  private static final int TEXT_EVENT_TARGET_LONG_PRESS = 1 << 2;
  private static final int TEXT_EVENT_TARGET_TOUCH_START = 1 << 3;
  private static final int TEXT_EVENT_TARGET_TOUCH_MOVE = 1 << 4;
  private static final int TEXT_EVENT_TARGET_TOUCH_END = 1 << 5;
  private static final int TEXT_EVENT_TARGET_TOUCH_CANCEL = 1 << 6;
  private static final int TEXT_SERVICE_EVENT_TARGET_INFO_SIZE = 3;

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
      Layout layout, Spanned spanned, PointF textTranslateOffset, Page textServicePage,
      boolean ignoreUserInteraction) {
    if (textServicePage != null) {
      return hitTestTextServicePage(ui, x, y, parent, textServicePage, ignoreUserInteraction);
    }
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

    return hitTestInlineView(ui, originX, originY, target, ignoreUserInteraction);
  }

  private static EventTarget hitTestTextServicePage(LynxBaseUI ui, float x, float y,
      EventTarget parent, Page textServicePage, boolean ignoreUserInteraction) {
    int[] targets = textServicePage.getHitTestEventTargets(x, y);
    if (targets == null || targets.length < TEXT_SERVICE_EVENT_TARGET_INFO_SIZE) {
      return parent;
    }
    int sign = targets[0];
    int eventMask = targets[1];
    boolean isInlineView = targets[2] != 0;
    if (!isInlineView && sign == parent.getSign()) {
      return parent;
    }

    Map<String, EventsListener> events = buildEventListeners(eventMask);
    EventTargetSpan target = isInlineView
        ? new InlineViewEventSpan(sign, events, EventTarget.EnableStatus.Undefined, true,
            EventTarget.EnableStatus.Undefined, EventTarget.PointerEventsValue.Unset,
            new JavaOnlyMap())
        : new EventTargetSpan(sign, events, EventTarget.EnableStatus.Undefined, true,
            EventTarget.EnableStatus.Undefined, EventTarget.PointerEventsValue.Unset,
            new JavaOnlyMap());
    target.setParent(parent);
    return hitTestInlineView(ui, x, y, target, ignoreUserInteraction);
  }

  private static EventTarget hitTestInlineView(
      LynxBaseUI ui, float x, float y, EventTarget target, boolean ignoreUserInteraction) {
    if (target instanceof InlineViewEventSpan) {
      for (LynxBaseUI childUI : ui.getChildren()) {
        if (childUI.getSign() == target.getSign()) {
          return childUI.hitTest(
              x - childUI.getOriginLeft(), y - childUI.getOriginTop(), ignoreUserInteraction);
        }
      }
    }
    return target;
  }

  private static Map<String, EventsListener> buildEventListeners(int eventMask) {
    if (eventMask == 0) {
      return null;
    }
    Map<String, EventsListener> events = new HashMap<>();
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_TAP, EVENT_TAP);
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_CLICK, EVENT_CLICK);
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_LONG_PRESS, EVENT_LONG_PRESS);
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_TOUCH_START, EVENT_TOUCH_START);
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_TOUCH_MOVE, EVENT_TOUCH_MOVE);
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_TOUCH_END, EVENT_TOUCH_END);
    addEventListener(events, eventMask, TEXT_EVENT_TARGET_TOUCH_CANCEL, EVENT_TOUCH_CANCEL);
    return events.isEmpty() ? null : events;
  }

  private static void addEventListener(
      Map<String, EventsListener> events, int eventMask, int event, String eventName) {
    if ((eventMask & event) == 0) {
      return;
    }
    events.put(eventName, null);
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

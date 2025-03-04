// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.text.Layout;
import android.text.style.LineBackgroundSpan;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import java.util.HashMap;

public class LynxTextBackgroundSpan implements LineBackgroundSpan {
  private int mSpanStart;
  private int mSpanEnd;
  private boolean mIsVisible = true;
  private HashMap<Integer, int[]> mLineStartEndPositions;

  private LynxBackground mBackground;

  @Override
  public void drawBackground(@NonNull Canvas canvas, @NonNull Paint paint, int left, int right,
      int top, int baseline, int bottom, @NonNull CharSequence text, int start, int end,
      int lineNumber) {
    if (mBackground == null || mLineStartEndPositions == null || !mIsVisible) {
      return;
    }

    if (mLineStartEndPositions.containsKey(lineNumber)) {
      int[] startEndPosition = mLineStartEndPositions.get(lineNumber);
      if (startEndPosition != null && startEndPosition.length == 2
          && mBackground.getDrawable() != null) {
        mBackground.getDrawable().setBounds(Math.min(startEndPosition[0], startEndPosition[1]), top,
            Math.max(startEndPosition[0], startEndPosition[1]), bottom);
        mBackground.getDrawable().draw(canvas);
      }
    }
  }

  public LynxTextBackgroundSpan(int spanStart, int spanEnd, LynxBackground background) {
    mSpanStart = spanStart;
    mSpanEnd = spanEnd;
    mBackground = background;
  }

  public void updateSpanPosition(Layout layout) {
    mLineStartEndPositions = new HashMap<>();
    for (int lineIndex = layout.getLineForOffset(mSpanStart);
         lineIndex <= layout.getLineForOffset(mSpanEnd); lineIndex++) {
      int lineStartIndex = layout.getLineStart(lineIndex);
      int lineEndIndex = layout.getLineEnd(lineIndex);
      if (mSpanStart > lineStartIndex && mSpanStart < lineEndIndex) {
        lineStartIndex = mSpanStart;
      }
      if (mSpanEnd > lineStartIndex && mSpanEnd < lineEndIndex) {
        lineEndIndex = mSpanEnd;
      }
      int startPosition = (int) layout.getPrimaryHorizontal(lineStartIndex);
      int endPosition = (int) layout.getPrimaryHorizontal(lineEndIndex);
      if (lineStartIndex == layout.getLineStart(lineIndex)) {
        startPosition = (int) layout.getLineLeft(lineIndex);
      }
      if (lineEndIndex == layout.getLineEnd(lineIndex)) {
        endPosition = (int) layout.getLineRight(lineIndex);
      }
      mLineStartEndPositions.put(lineIndex, new int[] {startPosition, endPosition});
    }
  }

  public void updateBackgroundStartEndIndex(int addLength) {
    mSpanStart += addLength;
    mSpanEnd += addLength;
  }

  public void updateBackgroundEndIndex(int endIndex) {
    if (mSpanStart >= endIndex) {
      mIsVisible = false;
      return;
    }

    if (mSpanEnd > endIndex) {
      mSpanEnd = endIndex;
    }
  }
}

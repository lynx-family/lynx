// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Path;
import android.graphics.PathMeasure;
import android.graphics.PointF;
import android.graphics.RectF;
import android.text.Layout;
import java.util.ArrayList;

/** Character-range rectangles from the measured StaticLayout, in paragraph content coordinates. */
public final class InlineTextGeometry {
  private InlineTextGeometry() {}

  public static float[] getRegions(Object data, int[] targetRanges) {
    if (!(data instanceof TextUpdateBundle) || targetRanges == null)
      return new float[0];
    TextUpdateBundle bundle = (TextUpdateBundle) data;
    Layout layout = bundle.getTextLayout();
    if (layout == null)
      return new float[0];
    int lineCount =
        bundle.hasLayoutEventParams() ? bundle.getLayoutEventLineCount() : layout.getLineCount();
    if (lineCount == 0) {
      return new float[0];
    }
    int layoutLength = layout.getText().length();
    CharSequence source = bundle.getOriginText();
    int sourceLength = source != null ? source.length() : layoutLength;
    int lastLine = lineCount - 1;
    int visibleEnd = Math.min(bundle.getSourceTextEnd(), layout.getLineEnd(lastLine));
    if (layout.getEllipsisCount(lastLine) > 0) {
      // Platform ellipsis retains the original offsets but draws the synthetic
      // glyph at the first hidden character.
      int ellipsisStart = layout.getLineStart(lastLine) + layout.getEllipsisStart(lastLine);
      visibleEnd = Math.min(visibleEnd, ellipsisStart);
    }
    int truncationStart = bundle.getInlineTruncationStart();
    ArrayList<Float> result = new ArrayList<>();
    for (int i = 0; i + 2 < targetRanges.length; i += 3) {
      int start = targetRanges[i + 1];
      int end = targetRanges[i + 2];
      if (start >= sourceLength && truncationStart >= 0) {
        // Token ranges follow the original body in the native input. In the
        // measured layout they follow only the retained body prefix.
        start -= sourceLength - truncationStart;
        end -= sourceLength - truncationStart;
      } else if (end <= sourceLength) {
        end = truncationStart >= 0 ? Math.min(end, truncationStart) : end;
      } else {
        continue;
      }
      appendLayoutRegions(result, targetRanges[i], layout, start, Math.min(end, visibleEnd),
          bundle.getTextTranslateOffset());
    }
    float[] regions = new float[result.size()];
    for (int i = 0; i < result.size(); ++i) {
      regions[i] = result.get(i);
    }
    return regions;
  }

  private static void appendTextEventTargetRegion(
      ArrayList<Float> result, int sign, float left, float top, float right, float bottom) {
    if (right <= left || bottom <= top) {
      return;
    }
    // Keep the full 32-bit sign while sharing one compact float array with geometry.
    result.add(Float.intBitsToFloat(sign));
    result.add(left);
    result.add(top);
    result.add(right - left);
    result.add(bottom - top);
  }

  public static void appendLayoutRegions(
      ArrayList<Float> result, int sign, Layout layout, int start, int end, PointF offset) {
    if (start < 0 || start >= end || end > layout.getText().length()) {
      return;
    }
    int startLine = layout.getLineForOffset(start);
    int endLine = layout.getLineForOffset(end - 1);
    float offsetX = offset != null ? offset.x : 0.f;
    float offsetY = offset != null ? offset.y : 0.f;
    Path selection = new Path();
    Path fragment = new Path();
    PathMeasure contours = new PathMeasure();
    RectF bounds = new RectF();
    for (int line = startLine; line <= endLine; ++line) {
      int lineStart = layout.getLineStart(line);
      int lineEnd = layout.getLineEnd(line);
      int rangeStart = Math.max(start, lineStart);
      int rangeEnd = Math.min(end, lineEnd);
      // Line terminators have no glyph area and must not extend a link to the
      // next line's origin.
      while (rangeEnd > rangeStart
          && (layout.getText().charAt(rangeEnd - 1) == '\n'
              || layout.getText().charAt(rangeEnd - 1) == '\r')) {
        --rangeEnd;
      }
      if (rangeStart >= rangeEnd) {
        continue;
      }
      // Layout emits one rectangular contour per directional run. A single pair
      // of primary horizontals can include unrelated glyphs or miss part of a run.
      layout.getSelectionPath(rangeStart, rangeEnd, selection);
      contours.setPath(selection, false);
      int top = layout.getLineTop(line);
      int bottom = layout.getLineBottom(line);
      boolean endsAtWrap = rangeEnd == lineEnd && line < layout.getLineCount() - 1;
      do {
        fragment.rewind();
        if (!contours.getSegment(0, contours.getLength(), fragment, true)) {
          continue;
        }
        fragment.computeBounds(bounds, true);
        if (bounds.bottom <= top || bounds.top >= bottom) {
          continue;
        }
        // If the end offset resolves to the next line, Layout adds selection
        // through the current line's trailing blank area.
        float left = endsAtWrap ? Math.max(bounds.left, layout.getLineLeft(line)) : bounds.left;
        float right = endsAtWrap ? Math.min(bounds.right, layout.getLineRight(line)) : bounds.right;
        appendTextEventTargetRegion(
            result, sign, left + offsetX, top + offsetY, right + offsetX, bottom + offsetY);
      } while (contours.nextContour());
    }
  }
}

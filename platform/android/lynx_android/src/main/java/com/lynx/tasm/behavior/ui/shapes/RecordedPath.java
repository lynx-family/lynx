// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.shapes;

import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.RectF;
import androidx.annotation.RestrictTo;
import java.nio.ByteBuffer;

/** Android geometry for one record in a Fragment display list. */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class RecordedPath {
  // Match PathKind in core/renderer/dom/fragment/path.h.
  private static final int COMMANDS = 0, CIRCLE = 1, ELLIPSE = 2, RECT = 3, SUPER_ELLIPSE = 5;
  private static final int COMMAND_BYTES = 28;
  private final RectF mRect;
  private final Path mPath;

  private RecordedPath(RectF rect, Path path) {
    mRect = rect;
    mPath = path;
  }

  public void clip(Canvas canvas) {
    if (mRect != null) {
      canvas.clipRect(mRect);
    } else {
      canvas.clipPath(mPath);
    }
  }

  // Invalid records are ignored, whereas a valid empty outline clips everything.
  public static RecordedPath decode(ByteBuffer items, int item, ByteBuffer data) {
    int offset = items.getInt(item + 4), size = items.getInt(item + 8);
    int kind = items.getInt(item + 12);
    if (offset < 0 || size < 0 || (long) offset + size > (data == null ? 0 : data.capacity())) {
      return null;
    }
    Path path = new Path();
    switch (kind) {
      case CIRCLE:
        if (size != 12)
          return null;
        path.addCircle(data.getFloat(offset), data.getFloat(offset + 4), data.getFloat(offset + 8),
            Path.Direction.CW);
        break;
      case ELLIPSE:
      case SUPER_ELLIPSE: {
        if (size != (kind == ELLIPSE ? 16 : 24))
          return null;
        float cx = data.getFloat(offset), cy = data.getFloat(offset + 4);
        float rx = data.getFloat(offset + 8), ry = data.getFloat(offset + 12);
        if (rx <= 0 || ry <= 0)
          break;
        if (kind == ELLIPSE) {
          path.addOval(cx - rx, cy - ry, cx + rx, cy + ry, Path.Direction.CW);
        } else {
          for (int q = 1; q <= 4; q++) {
            BasicShape.addLameCurveToPath(
                path, rx, ry, cx, cy, data.getFloat(offset + 16), data.getFloat(offset + 20), q);
          }
          path.close();
        }
        break;
      }
      case RECT: {
        if (size != 56)
          return null;
        float left = data.getFloat(offset), top = data.getFloat(offset + 4);
        RectF bounds = new RectF(
            left, top, left + data.getFloat(offset + 8), top + data.getFloat(offset + 12));
        float[] radii = null;
        // RoundedRectangle stores X and Y separately; Android expects XY pairs.
        for (int corner = 0; corner < 4; corner++) {
          float rx = data.getFloat(offset + 16 + 4 * corner);
          float ry = data.getFloat(offset + 32 + 4 * corner);
          if (rx > 0 && ry > 0) {
            if (radii == null)
              radii = new float[8];
            radii[2 * corner] = rx;
            radii[2 * corner + 1] = ry;
          }
        }
        if (radii == null || bounds.isEmpty())
          return new RecordedPath(bounds, null);
        float ex = data.getFloat(offset + 48), ey = data.getFloat(offset + 52);
        if (ex == 2 && ey == 2) {
          path.addRoundRect(bounds, radii, Path.Direction.CW);
        } else {
          addSuperCorners(path, bounds, radii, ex, ey);
        }
        break;
      }
      case COMMANDS:
        if (size % COMMAND_BYTES != 0)
          return null;
        for (int p = offset; p < offset + size; p += COMMAND_BYTES) {
          float x = data.getFloat(p + 4), y = data.getFloat(p + 8);
          switch (data.getInt(p)) {
            case 0:
              path.moveTo(x, y);
              break;
            case 1:
              path.lineTo(x, y);
              break;
            case 2:
              path.cubicTo(data.getFloat(p + 12), data.getFloat(p + 16), data.getFloat(p + 20),
                  data.getFloat(p + 24), x, y);
              break;
            case 3:
              path.close();
              break;
            default:
              return null;
          }
        }
        break;
      default:
        return null;
    }
    if (items.getInt(item + 16) == 1)
      path.setFillType(Path.FillType.EVEN_ODD);
    return new RecordedPath(null, path);
  }

  private static void addSuperCorners(Path path, RectF b, float[] r, float ex, float ey) {
    // Scale overlapping corner radii together, as for an ordinary rounded rect.
    float scale = Math.min(1f,
        Math.min(b.width() / Math.max(r[0] + r[2], r[4] + r[6]),
            b.height() / Math.max(r[1] + r[7], r[3] + r[5])));
    for (int q = 1; q <= 4; q++) {
      int i = ((q + 1) % 4) * 2; // BR, BL, TL, TR
      float rx = r[i] * scale, ry = r[i + 1] * scale;
      float cx = q == 1 || q == 4 ? b.right - rx : b.left + rx;
      float cy = q <= 2 ? b.bottom - ry : b.top + ry;
      BasicShape.addLameCurveToPath(path, rx, ry, cx, cy, ex, ey, q);
    }
    path.close();
  }
}

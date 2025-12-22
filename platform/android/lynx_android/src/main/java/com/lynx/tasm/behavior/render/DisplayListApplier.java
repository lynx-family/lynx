// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.text.Layout;
import android.text.Spanned;
import android.view.View;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import com.lynx.tasm.behavior.ui.utils.BorderDrawingUtil;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Stack;

public class DisplayListApplier implements Drawable.Callback {
  private static String TAG = "DisplayListApplier";

  // Operation type constants matching C++ DisplayListOpType and DisplayListSubtreePropertyOpType
  private static final int OP_BEGIN = 0;
  private static final int OP_END = 1;
  private static final int OP_FILL = 2;
  private static final int OP_DRAW_VIEW = 3;
  private static final int OP_TEXT = 6;
  private static final int OP_IMAGE = 7;
  private static final int OP_CUSTOM = 8;
  private static final int OP_BORDER = 9;
  private static final int OP_CLIP_RECT = 10;
  private static final int OP_RECORD_BOX = 11;

  private DisplayList mDisplayList;
  private TextMeasurer mTextMeasurer;
  private Paint mPaint;
  private Stack<RectF> mBounds;

  private PlatformRendererContext mContext;
  // Separate indices for content and subtree property operations
  private int mContentOpIndex;
  private int mContentIntIndex;
  private int mContentFloatIndex;

  private WeakReference<View> mHostLayer;

  private final ArrayList<RoundedRectangle> mRoundedRectangleArray = new ArrayList<>();

  public DisplayListApplier(
      DisplayList displayList, PlatformRendererContext platformRendererContext, View hostLayer) {
    mDisplayList = displayList;
    mPaint = new Paint();
    mPaint.setAntiAlias(true);
    mBounds = new Stack<>();
    reset();
    mTextMeasurer = platformRendererContext.getTextMeasurer();
    mContext = platformRendererContext;
    mHostLayer = new WeakReference<>(hostLayer);
  }

  public void reset() {
    mContentOpIndex = 0;
    mContentIntIndex = 0;
    mContentFloatIndex = 0;
    mBounds.clear();
    mRoundedRectangleArray.clear();
  }

  public void drawTillNextView(Canvas canvas) {
    if (mDisplayList == null) {
      return;
    }

    // Process content operations
    processContentOperations(canvas);
  }

  private void drawImage(Canvas canvas, int id, int boxIndex) {
    LynxImageManager imageManager = mContext.getImage(id);
    if (imageManager == null) {
      return;
    }
    imageManager.updateInnerClipPathForBorderRadius(
        boxIndex >= 0 ? mRoundedRectangleArray.get(boxIndex) : null);
    imageManager.setView(mHostLayer.get());
    imageManager.onDraw(canvas);
  }

  private void drawText(Canvas canvas, int textId) {
    TextUpdateBundle textBundle = (TextUpdateBundle) mTextMeasurer.takeTextLayout(textId);
    if (textBundle == null) {
      return;
    }

    if (textBundle.hasImages()) {
      updateInlineImageSpans(textBundle);
    }

    Layout textLayout = textBundle.getTextLayout();
    if (textLayout != null) {
      PointF offset = textBundle.getTextTranslateOffset();
      if (offset != null) {
        canvas.translate(offset.x, offset.y);
      }
      textLayout.draw(canvas);
    }
  }

  private void updateInlineImageSpans(TextUpdateBundle textBundle) {
    Layout layout = textBundle.getTextLayout();
    if (layout == null) {
      return;
    }

    CharSequence text = layout.getText();
    if (text instanceof Spanned) {
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans((Spanned) text, this);
    }
  }

  void recordRoundedRectangle(RoundedRectangle roundedRectangle) {
    mRoundedRectangleArray.add(roundedRectangle);
  }

  private void processContentOperations(Canvas canvas) {
    if (mDisplayList.ops == null) {
      return;
    }

    while (mContentOpIndex < mDisplayList.ops.length) {
      // Read operation type and parameter counts
      if (mContentIntIndex >= mDisplayList.iArgv.length) {
        break;
      }

      int op = mDisplayList.ops[mContentOpIndex++];
      int intParamCount = mDisplayList.iArgv[mContentIntIndex++];
      int floatParamCount = mDisplayList.iArgv[mContentIntIndex++];

      switch (op) {
        case OP_BEGIN:
          // Begin fragment: id, x, y, width, height (1 int, 4 floats)
          if (intParamCount == 1) {
            long id = nextContentInt();
          }
          if (floatParamCount == 4) {
            float x = nextContentFloat();
            float y = nextContentFloat();
            float width = nextContentFloat();
            float height = nextContentFloat();
            mBounds.push(new RectF(0, 0, width, height));
            canvas.save();
            canvas.translate(x, y);
            // No direct canvas operation, just consume parameters
          }
          break;

        case OP_END:
          // End fragment - no parameters
          canvas.restore();
          mBounds.pop();
          break; // End of this sub view's content

        case OP_FILL:
          mPaint.reset();
          if (intParamCount == 1) {
            // Fill: color (1 int)
            int color = nextContentInt();
            // This would fill the entire fragment bound
            mPaint.setColor(color);
            // Need to get bounds from the Begin operation or context
            canvas.drawRect(mBounds.peek(), mPaint);
          }
          break;

        case OP_DRAW_VIEW:
          // Draw view: view_id (1 int)
          if (intParamCount == 1) {
            int viewId = nextContentInt();
            // This indicates we should stop processing and let the view draw itself
          }
          return;

        case OP_TEXT:
          // Text: id (1 int)
          if (intParamCount >= 1) {
            int textId = nextContentInt();
            drawText(canvas, textId);
          }
          break;

        case OP_IMAGE:
          // Image: image_id (1 int)
          if (intParamCount == 2) {
            int imageId = nextContentInt();
            int boxIndex = nextContentInt();
            drawImage(canvas, imageId, boxIndex);
          }
          break;
        case OP_BORDER:
          if (intParamCount == 10) {
            int outBoxIndex = nextContentInt();
            int innerBoxIndex = nextContentInt();

            // 8 ints: border colors (4) + border styles (4)
            int[] borderColors = new int[4];

            borderColors[Spacing.TOP] = nextContentInt(); // top color
            borderColors[Spacing.RIGHT] = nextContentInt(); // right color
            borderColors[Spacing.BOTTOM] = nextContentInt(); // bottom color
            borderColors[Spacing.LEFT] = nextContentInt(); // left color

            BorderStyle[] borderStyles = new BorderStyle[4];

            borderStyles[Spacing.TOP] = BorderStyle.parse(nextContentInt()); // top style
            borderStyles[Spacing.RIGHT] = BorderStyle.parse(nextContentInt()); // right style
            borderStyles[Spacing.BOTTOM] = BorderStyle.parse(nextContentInt()); // bottom style
            borderStyles[Spacing.LEFT] = BorderStyle.parse(nextContentInt()); // left style

            // Use the member function to draw borders (verifiable in tests)
            drawRectangularBorders(
                canvas, mPaint, outBoxIndex, innerBoxIndex, borderColors, borderStyles);
          }
          break;
        case OP_CLIP_RECT: {
          float left = nextContentFloat();
          float top = nextContentFloat();
          float width = nextContentFloat();
          float height = nextContentFloat();

          RectF rectF = new RectF(left, top, left + width, top + height);
          if (floatParamCount > 4) {
            float[] borderRadii = new float[8];
            borderRadii[0] = nextContentFloat(); // top left x
            borderRadii[1] = nextContentFloat(); // top left y
            borderRadii[2] = nextContentFloat(); // top right x
            borderRadii[3] = nextContentFloat(); // top right y
            borderRadii[4] = nextContentFloat(); // bottom right x
            borderRadii[5] = nextContentFloat(); // bottom right y
            borderRadii[6] = nextContentFloat(); // bottom left x
            borderRadii[7] = nextContentFloat(); // bottom left y

            Path path = new Path();
            path.addRoundRect(rectF, borderRadii, Path.Direction.CW);
            canvas.clipPath(path);
          } else {
            canvas.clipRect(rectF);
          }
          break;
        }
        case OP_RECORD_BOX: {
          float left = nextContentFloat();
          float top = nextContentFloat();
          float width = nextContentFloat();
          float height = nextContentFloat();

          RectF rectF = new RectF(left, top, left + width, top + height);
          float[] borderRadii = null;
          if (floatParamCount > 4) {
            borderRadii = new float[8];
            borderRadii[0] = nextContentFloat(); // top left x
            borderRadii[1] = nextContentFloat(); // top left y
            borderRadii[2] = nextContentFloat(); // top right x
            borderRadii[3] = nextContentFloat(); // top right y
            borderRadii[4] = nextContentFloat(); // bottom right x
            borderRadii[5] = nextContentFloat(); // bottom right y
            borderRadii[6] = nextContentFloat(); // bottom left x
            borderRadii[7] = nextContentFloat(); // bottom left y
          }
          recordRoundedRectangle(new RoundedRectangle(rectF, borderRadii));
          break;
        }
        default:
          break;
      }
    }
  }

  /**
   * Draws rectangular borders by delegating to BorderDrawingUtil.
   * This method exists to make border drawing verifiable in unit tests.
   *
   * @param canvas The canvas to draw on
   * @param paint The paint object to use for drawing
   * @param bounds The bounds of the rectangle to draw borders around
   * @param borderWidths Array of border widths for [left, top, right, bottom] in pixels
   * @param borderColors Array of border colors for [left, top, right, bottom]
   * @param borderStyles Array of border styles for [left, top, right, bottom]
   */
  void drawRectangularBorders(Canvas canvas, Paint paint, int outBoxIndex, int innerBoxIndex,
      int[] borderColors, BorderStyle[] borderStyles) {
    RoundedRectangle outBox = null;
    if (outBoxIndex >= 0 && outBoxIndex < mRoundedRectangleArray.size()) {
      outBox = mRoundedRectangleArray.get(outBoxIndex);
    }

    RoundedRectangle innerBox = null;
    if (innerBoxIndex >= 0 && innerBoxIndex < mRoundedRectangleArray.size()) {
      innerBox = mRoundedRectangleArray.get(innerBoxIndex);
    }

    if (outBox == null || innerBox == null) {
      LLog.e(TAG, "drawRectangularBorders failed since outBox or innerBox is null.");
      return;
    }

    BorderDrawingUtil.drawBorders(canvas, paint, outBox, innerBox, borderColors, borderStyles);
  }

  // Helper methods for reading content data
  private int nextContentInt() {
    if (mDisplayList.iArgv != null && mContentIntIndex < mDisplayList.iArgv.length) {
      return mDisplayList.iArgv[mContentIntIndex++];
    }
    return 0;
  }

  private float nextContentFloat() {
    if (mDisplayList.fArgv != null && mContentFloatIndex < mDisplayList.fArgv.length) {
      return mDisplayList.fArgv[mContentFloatIndex++];
    }
    return 0.0f;
  }

  public void setDisplayList(DisplayList displayList) {
    mDisplayList = displayList;
    reset();
  }

  @Override
  public void invalidateDrawable(@NonNull Drawable who) {
    View hostLayer = mHostLayer.get();
    if (hostLayer != null) {
      hostLayer.invalidate();
    }
  }

  @Override
  public void scheduleDrawable(@NonNull Drawable who, @NonNull Runnable what, long when) {}

  @Override
  public void unscheduleDrawable(@NonNull Drawable who, @NonNull Runnable what) {}
}

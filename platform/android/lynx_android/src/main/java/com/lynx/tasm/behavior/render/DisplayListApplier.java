// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.Shader;
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
  private static final int OP_LINEAR_GRADIENT = 12;

  private DisplayList mDisplayList;
  private TextMeasurer mTextMeasurer;
  private Paint mPaint;

  // Reusable objects for optimization
  private final Path mReusablePath = new Path();
  private final RectF mReusableRectF = new RectF();
  private final float[] mReusableBorderRadii = new float[8];
  private final int[] mReusableBorderColors = new int[4];
  private final BorderStyle[] mReusableBorderStyles = new BorderStyle[4];
  private final PointF mReusablePointF1 = new PointF();
  private final PointF mReusablePointF2 = new PointF();
  private final PointF mReusablePointF3 = new PointF();
  private final PointF mReusablePointF4 = new PointF();
  private int[] mReusableGradientColors;
  private float[] mReusableGradientStops;

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
    reset();
    mTextMeasurer = platformRendererContext.getTextMeasurer();
    mContext = platformRendererContext;
    mHostLayer = new WeakReference<>(hostLayer);
  }

  public void reset() {
    mContentOpIndex = 0;
    mContentIntIndex = 0;
    mContentFloatIndex = 0;
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
    if (mDisplayList.ops == null || mDisplayList.iArgv == null) {
      return;
    }

    final int[] ops = mDisplayList.ops;
    final int[] iArgv = mDisplayList.iArgv;
    final float[] fArgv = mDisplayList.fArgv;
    final int opsLength = ops.length;
    final int iArgvLength = iArgv.length;

    while (mContentOpIndex < opsLength) {
      // Read operation type and parameter counts
      if (mContentIntIndex + 1 >= iArgvLength) {
        break;
      }

      int op = ops[mContentOpIndex++];
      int intParamCount = iArgv[mContentIntIndex++];
      int floatParamCount = iArgv[mContentIntIndex++];

      if (intParamCount < 0 || floatParamCount < 0) {
        LLog.e(TAG, "Invalid param count: " + intParamCount + ", " + floatParamCount);
        break;
      }

      int nextIntIndex = mContentIntIndex + intParamCount;
      int nextFloatIndex = mContentFloatIndex + floatParamCount;

      switch (op) {
        case OP_BEGIN: {
          // Begin fragment: id, x, y, width, height (1 int, 4 floats)
          if (intParamCount >= 1) {
            nextContentInt(); // skip id
          }
          if (floatParamCount >= 4) {
            float x = nextContentFloat();
            float y = nextContentFloat();
            nextContentFloat(); // unused width
            nextContentFloat(); // unused height
            canvas.save();
            canvas.translate(x, y);
          }
          break;
        }

        case OP_END: {
          // End fragment - no parameters
          canvas.restore();
          break; // End of this sub view's content
        }

        case OP_FILL: {
          mPaint.reset();
          // Fill: color (1 int), clip_index (1 int)
          int color = nextContentInt();
          int clipIndex = nextContentInt();
          mPaint.setColor(color);

          if (clipIndex >= 0 && clipIndex < mRoundedRectangleArray.size()) {
            RoundedRectangle roundedRectangle = mRoundedRectangleArray.get(clipIndex);
            if (roundedRectangle.hasBorderRadius()) {
              mReusablePath.reset();
              mReusablePath.addRoundRect(roundedRectangle.getRectF(),
                  roundedRectangle.getBorderRadii(), Path.Direction.CW);
              canvas.drawPath(mReusablePath, mPaint);
            } else {
              canvas.drawRect(roundedRectangle.getRectF(), mPaint);
            }
          }
          break;
        }

        case OP_DRAW_VIEW: {
          // Draw view: view_id (1 int)
          nextContentInt();
          return;
        }

        case OP_TEXT: {
          // Text: id (1 int)
          if (intParamCount >= 2) {
            int textId = nextContentInt();
            // TODO(songshourui.null): Android doesn't use this index for now.
            int boxIndex = nextContentInt();
            drawText(canvas, textId);
          }
          break;
        }

        case OP_IMAGE: {
          // Image: image_id (1 int), boxIndex (1 int)
          if (intParamCount >= 2) {
            int imageId = nextContentInt();
            int boxIndex = nextContentInt();
            drawImage(canvas, imageId, boxIndex);
          }
          break;
        }
        case OP_BORDER: {
          if (intParamCount >= 10) {
            mPaint.reset();
            mPaint.setAntiAlias(true);
            int outBoxIndex = nextContentInt();
            int innerBoxIndex = nextContentInt();

            // 8 ints: border colors (4) + border styles (4)
            // Order from C++ is: Top, Right, Bottom, Left
            mReusableBorderColors[Spacing.TOP] = nextContentInt();
            mReusableBorderColors[Spacing.RIGHT] = nextContentInt();
            mReusableBorderColors[Spacing.BOTTOM] = nextContentInt();
            mReusableBorderColors[Spacing.LEFT] = nextContentInt();

            mReusableBorderStyles[Spacing.TOP] = BorderStyle.parse(nextContentInt());
            mReusableBorderStyles[Spacing.RIGHT] = BorderStyle.parse(nextContentInt());
            mReusableBorderStyles[Spacing.BOTTOM] = BorderStyle.parse(nextContentInt());
            mReusableBorderStyles[Spacing.LEFT] = BorderStyle.parse(nextContentInt());

            // Use the member function to draw borders (verifiable in tests)
            drawRectangularBorders(canvas, mPaint, outBoxIndex, innerBoxIndex,
                mReusableBorderColors, mReusableBorderStyles);
          }
          break;
        }
        case OP_CLIP_RECT: {
          float left = nextContentFloat();
          float top = nextContentFloat();
          float width = nextContentFloat();
          float height = nextContentFloat();

          mReusableRectF.set(left, top, left + width, top + height);
          if (floatParamCount >= 12) { // 4 + 8 radii
            System.arraycopy(fArgv, mContentFloatIndex, mReusableBorderRadii, 0, 8);
            mContentFloatIndex += 8;

            mReusablePath.reset();
            mReusablePath.addRoundRect(mReusableRectF, mReusableBorderRadii, Path.Direction.CW);
            canvas.clipPath(mReusablePath);
          } else {
            canvas.clipRect(mReusableRectF);
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
          if (floatParamCount >= 12) {
            borderRadii = new float[8];
            System.arraycopy(fArgv, mContentFloatIndex, borderRadii, 0, 8);
            mContentFloatIndex += 8;
          }
          recordRoundedRectangle(new RoundedRectangle(rectF, borderRadii));
          break;
        }
        case OP_LINEAR_GRADIENT: {
          int colorCount = nextContentInt();
          if (mReusableGradientColors == null || mReusableGradientColors.length != colorCount) {
            mReusableGradientColors = new int[colorCount];
          }
          int[] colors = mReusableGradientColors;
          System.arraycopy(iArgv, mContentIntIndex, colors, 0, colorCount);
          mContentIntIndex += colorCount;

          int stopCount = nextContentInt();
          int tilingIndex = nextContentInt();
          int clipIndex = nextContentInt();
          int repeatX = nextContentInt();
          int repeatY = nextContentInt();

          float angle = nextContentFloat();
          float[] stops = null;
          if (stopCount > 0) {
            if (mReusableGradientStops == null || mReusableGradientStops.length != stopCount) {
              mReusableGradientStops = new float[stopCount];
            }
            stops = mReusableGradientStops;
            System.arraycopy(fArgv, mContentFloatIndex, stops, 0, stopCount);
            mContentFloatIndex += stopCount;
          }

          drawLinearGradient(
              canvas, angle, colors, stops, tilingIndex, clipIndex, repeatX, repeatY);
          break;
        }
        default:
          break;
      }

      // Ensure alignment
      mContentIntIndex = nextIntIndex;
      mContentFloatIndex = nextFloatIndex;
    }
  }

  private void drawLinearGradient(Canvas canvas, float angle, int[] colors, float[] stops,
      int tilingIndex, int clipIndex, int repeatX, int repeatY) {
    if (colors == null) {
      return;
    }

    RoundedRectangle tilingBox = null;
    if (tilingIndex >= 0 && tilingIndex < mRoundedRectangleArray.size()) {
      tilingBox = mRoundedRectangleArray.get(tilingIndex);
    }
    if (tilingBox == null) {
      return;
    }

    float width = tilingBox.getRectF().width();
    float height = tilingBox.getRectF().height();
    float left = tilingBox.getRectF().left;
    float top = tilingBox.getRectF().top;

    PointF center = mReusablePointF1;
    center.set(left + width / 2.f, top + height / 2.f);
    double radial = Math.toRadians(angle);
    float sin = (float) Math.sin(radial);
    float cos = (float) Math.cos(radial);
    float tan = (float) Math.tan(radial);

    PointF m = mReusablePointF2;
    if (sin >= 0 && cos >= 0) { // Bottom left to top right
      m.set(left + width, top);
    } else if (sin >= 0 && cos < 0) { // Top left to bottom right
      m.set(left + width, top + height);
    } else if (sin < 0 && cos < 0) { // Top right to bottom left
      m.set(left, top + height);
    } else { // Bottom right to top left
      m.set(left, top);
    }

    // Reference logic from BackgroundLinearGradientLayer.java
    float tmp = (center.y - m.y - tan * center.x + tan * m.x);
    float endX = center.x + sin * tmp / (sin * tan + cos);
    float endY = center.y - tmp / (tan * tan + 1);
    float startX = 2 * center.x - endX;
    float startY = 2 * center.y - endY;

    mPaint.reset();
    mPaint.setAntiAlias(true);

    Shader.TileMode tileMode = Shader.TileMode.CLAMP;
    // BackgroundRepeatType: kRepeat = 0, kNoRepeat = 1, kRepeatX = 2, kRepeatY = 3, kRound = 4,
    // kSpace = 5 For LinearGradient, if either axis repeats, we use REPEAT tile mode.
    if (repeatX != 1 || repeatY != 1) {
      tileMode = Shader.TileMode.REPEAT;
    }

    mPaint.setShader(new LinearGradient(startX, startY, endX, endY, colors, stops, tileMode));

    if (clipIndex >= 0 && clipIndex < mRoundedRectangleArray.size()) {
      RoundedRectangle clipBox = mRoundedRectangleArray.get(clipIndex);
      if (clipBox.hasBorderRadius()) {
        mReusablePath.reset();
        mReusablePath.addRoundRect(clipBox.getRectF(), clipBox.getBorderRadii(), Path.Direction.CW);
        canvas.drawPath(mReusablePath, mPaint);
      } else {
        canvas.drawRect(clipBox.getRectF(), mPaint);
      }
    }
    mPaint.setShader(null);
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

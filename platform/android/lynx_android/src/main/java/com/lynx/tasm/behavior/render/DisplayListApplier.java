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
import com.lynx.tasm.service.ILynxTextService.Page;
import com.lynx.tasm.utils.GradientUtils;
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

  // BackgroundRepeatType constants (match C++ BackgroundRepeatType)
  private static final int BACKGROUND_REPEAT = 0;
  private static final int BACKGROUND_NO_REPEAT = 1;

  // Subtree property operation types (matching C++ DisplayListSubtreePropertyOpType)
  static final int SUBTREE_OP_TRANSFORM = 0;
  static final int SUBTREE_OP_OPACITY = 1;

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

    // The drawing position on Android is affected by the frame layout and the
    // frame in OP_BEGIN togather. For a indepent layer, its position is already
    // shifted by the layers layout frame, and avoid doing it again in OP_BEGIN.
    if (displayList != null && displayList.fArgv != null && displayList.fArgv.length >= 2) {
      displayList.fArgv[0] = displayList.fArgv[1] = 0.f;
    }
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
    RoundedRectangle rect = boxIndex >= 0 && boxIndex < mRoundedRectangleArray.size()
        ? mRoundedRectangleArray.get(boxIndex)
        : null;
    if (rect != null) {
      imageManager.updateDrawableBounds(rect.getRect());
    }
    imageManager.updateInnerClipPathForBorderRadius(rect);
    imageManager.setView(mHostLayer.get());
    imageManager.onDraw(canvas);
  }

  private void drawText(Canvas canvas, int textId) {
    if (mContext.getLynxContext() != null && mContext.getLynxContext().isTextServiceModeOn()) {
      drawTextForTextra(canvas, textId);
      return;
    }
    if (mTextMeasurer == null) {
      return;
    }
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

  private void drawTextForTextra(Canvas canvas, int textId) {
    Page page = mContext.getTextBundle(textId);
    if (page == null) {
      return;
    }
    page.drawPageCanvas(canvas, this);
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
          // Validate color count to prevent crashes or excessive memory allocation
          if (colorCount < 2 || colorCount > 256) {
            LLog.e(TAG, "Invalid color count for linear gradient: " + colorCount);
            break;
          }
          // Check bounds before array copy
          if (mContentIntIndex + colorCount > iArgv.length) {
            LLog.e(TAG, "Insufficient data for gradient colors");
            break;
          }
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

          // Validate stop count matches color count
          if (stopCount != 0 && stopCount != colorCount) {
            LLog.e(TAG,
                "Stop count (" + stopCount + ") doesn't match color count (" + colorCount + ")");
            break;
          }

          // Validate float data bounds
          if (mContentFloatIndex + 1 + stopCount > fArgv.length) {
            LLog.e(TAG, "Insufficient float data for gradient");
            break;
          }

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
    if (colors == null || colors.length < 2) {
      return;
    }

    RoundedRectangle tilingBox = null;
    if (tilingIndex >= 0 && tilingIndex < mRoundedRectangleArray.size()) {
      tilingBox = mRoundedRectangleArray.get(tilingIndex);
    }
    if (tilingBox == null) {
      return;
    }

    // Get clip box for clipping
    RoundedRectangle clipBox = null;
    if (clipIndex >= 0 && clipIndex < mRoundedRectangleArray.size()) {
      clipBox = mRoundedRectangleArray.get(clipIndex);
    }

    // Calculate gradient start and end points based on angle
    RectF tilingRect = tilingBox.getRectF();
    float width = tilingRect.width();
    float height = tilingRect.height();

    // Center point of the tiling box
    float centerX = tilingRect.centerX();
    float centerY = tilingRect.centerY();

    // Calculate gradient line based on angle
    // The gradient line should span the entire box at the given angle
    PointF start = mReusablePointF1;
    PointF end = mReusablePointF2;
    calculateGradientLine(centerX, centerY, width, height, angle, start, end);

    // For gradients, we use CLAMP for the gradient direction itself
    // and handle repeat by drawing multiple tiles
    LinearGradient gradient =
        new LinearGradient(start.x, start.y, end.x, end.y, colors, stops, Shader.TileMode.CLAMP);

    mPaint.reset();
    mPaint.setAntiAlias(true);
    mPaint.setShader(gradient);

    int saveCount = canvas.save();

    // Apply clip if specified
    if (clipBox != null) {
      RectF clipRect = clipBox.getRectF();
      if (clipBox.hasBorderRadius()) {
        mReusablePath.reset();
        mReusablePath.addRoundRect(clipRect, clipBox.getBorderRadii(), Path.Direction.CW);
        canvas.clipPath(mReusablePath);
      } else {
        canvas.clipRect(clipRect);
      }
    }

    // Draw the gradient
    // If repeat is enabled, we need to tile the gradient
    boolean repeatXEnabled = (repeatX == BACKGROUND_REPEAT);
    boolean repeatYEnabled = (repeatY == BACKGROUND_REPEAT);

    if (!repeatXEnabled && !repeatYEnabled) {
      // No repeat - draw gradient once within tiling box
      canvas.drawRect(tilingRect, mPaint);
    } else {
      // Handle repeat by tiling
      // Calculate the bounds for tiling (use clip box or canvas bounds)
      float left = (clipBox != null) ? clipBox.getRectF().left : tilingRect.left;
      float top = (clipBox != null) ? clipBox.getRectF().top : tilingRect.top;
      float right = (clipBox != null) ? clipBox.getRectF().right : tilingRect.right;
      float bottom = (clipBox != null) ? clipBox.getRectF().bottom : tilingRect.bottom;

      // Extend bounds for repeating
      if (repeatXEnabled && repeatYEnabled) {
        // Both X and Y repeat
        float startX = tilingRect.left;
        while (startX > left) {
          startX -= width;
        }
        float startY = tilingRect.top;
        while (startY > top) {
          startY -= height;
        }
        for (float x = startX; x < right; x += width) {
          for (float y = startY; y < bottom; y += height) {
            canvas.drawRect(x, y, x + width, y + height, mPaint);
          }
        }
      } else if (repeatXEnabled) {
        // Only X repeat
        float startX = tilingRect.left;
        while (startX > left) {
          startX -= width;
        }
        for (float x = startX; x < right; x += width) {
          canvas.drawRect(x, tilingRect.top, x + width, tilingRect.bottom, mPaint);
        }
      } else if (repeatYEnabled) {
        // Only Y repeat
        float startY = tilingRect.top;
        while (startY > top) {
          startY -= height;
        }
        for (float y = startY; y < bottom; y += height) {
          canvas.drawRect(tilingRect.left, y, tilingRect.right, y + height, mPaint);
        }
      }
    }

    canvas.restoreToCount(saveCount);
  }

  /**
   * Calculates the start and end points for a linear gradient line.
   * The gradient line passes through the center of the box and extends
   * to the edges based on the given angle.
   */
  private void calculateGradientLine(float centerX, float centerY, float width, float height,
      float angle, PointF start, PointF end) {
    // Use shared utility for consistent cross-platform gradient calculation
    GradientUtils.calculateGradientLine(angle, width, height, start, end);
    // Convert from local box coordinates (origin at center) to absolute coordinates
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    start.x += centerX - halfWidth;
    start.y += centerY - halfHeight;
    end.x += centerX - halfWidth;
    end.y += centerY - halfHeight;
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
    if (displayList != null && displayList.fArgv != null && displayList.fArgv.length >= 2) {
      displayList.fArgv[0] = displayList.fArgv[1] = 0.f;
    }
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

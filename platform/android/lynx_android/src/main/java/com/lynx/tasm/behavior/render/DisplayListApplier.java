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
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import com.lynx.tasm.behavior.ui.utils.BorderDrawingUtil;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import com.lynx.tasm.service.ILynxTextService.Page;
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
  private int mFragmentDepth;

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
    mFragmentDepth = 0;
    mRoundedRectangleArray.clear();
  }

  public void drawTillNextView(Canvas canvas) {
    if (mDisplayList == null) {
      return;
    }

    // Process content operations
    processContentOperations(canvas);
  }

  private static final class BorderBoxes {
    final RoundedRectangle outBox;
    final RoundedRectangle innerBox;

    BorderBoxes(RoundedRectangle outBox, RoundedRectangle innerBox) {
      this.outBox = outBox;
      this.innerBox = innerBox;
    }
  }

  private View getHostLayer() {
    return mHostLayer.get();
  }

  private boolean shouldNormalizeRootGeometry() {
    View hostLayer = getHostLayer();
    return mFragmentDepth == 1 && hostLayer != null && hostLayer.getWidth() > 0
        && hostLayer.getHeight() > 0;
  }

  private RectF getHostBoundsRectF(boolean includeScrollOffset) {
    View hostLayer = getHostLayer();
    if (hostLayer == null) {
      return new RectF();
    }

    float left = 0.f;
    float top = 0.f;
    if (includeScrollOffset && hostLayer instanceof AndroidScrollView) {
      left = hostLayer.getScrollX();
      top = hostLayer.getScrollY();
    }
    return new RectF(left, top, left + hostLayer.getWidth(), top + hostLayer.getHeight());
  }

  private float[] cloneBorderRadii(float[] borderRadii) {
    return borderRadii == null ? null : borderRadii.clone();
  }

  private RoundedRectangle createRoundedRectangle(RectF rect, RoundedRectangle source) {
    return new RoundedRectangle(new RectF(rect), cloneBorderRadii(source.getBorderRadii()));
  }

  private RoundedRectangle getRoundedRectangle(int index) {
    if (index < 0 || index >= mRoundedRectangleArray.size()) {
      return null;
    }
    return mRoundedRectangleArray.get(index);
  }

  private RoundedRectangle getNormalizedRoundedRectangle(int index) {
    RoundedRectangle roundedRectangle = getRoundedRectangle(index);
    if (roundedRectangle == null || !shouldNormalizeRootGeometry()) {
      return roundedRectangle;
    }

    RectF normalizedRect = new RectF(roundedRectangle.getRectF());
    if (!normalizedRect.intersect(getHostBoundsRectF(false))) {
      normalizedRect.setEmpty();
    }
    if (normalizedRect.equals(roundedRectangle.getRectF())) {
      return roundedRectangle;
    }
    return createRoundedRectangle(normalizedRect, roundedRectangle);
  }

  private BorderBoxes getNormalizedBorderBoxes(int outBoxIndex, int innerBoxIndex) {
    RoundedRectangle outBox = getRoundedRectangle(outBoxIndex);
    RoundedRectangle innerBox = getRoundedRectangle(innerBoxIndex);
    if (outBox == null || innerBox == null || !shouldNormalizeRootGeometry()) {
      return new BorderBoxes(outBox, innerBox);
    }

    RectF rawOutRect = outBox.getRectF();
    RectF rawInnerRect = innerBox.getRectF();
    RectF hostBounds = getHostBoundsRectF(false);
    RectF normalizedOutRect = new RectF(rawOutRect);
    if (!normalizedOutRect.intersect(hostBounds)) {
      normalizedOutRect.setEmpty();
    }

    if (normalizedOutRect.equals(rawOutRect)) {
      return new BorderBoxes(outBox, innerBox);
    }

    float borderLeftWidth = Math.max(rawInnerRect.left - rawOutRect.left, 0.f);
    float borderTopWidth = Math.max(rawInnerRect.top - rawOutRect.top, 0.f);
    float borderRightWidth = Math.max(rawOutRect.right - rawInnerRect.right, 0.f);
    float borderBottomWidth = Math.max(rawOutRect.bottom - rawInnerRect.bottom, 0.f);

    RectF normalizedInnerRect =
        new RectF(Math.min(normalizedOutRect.left + borderLeftWidth, normalizedOutRect.right),
            Math.min(normalizedOutRect.top + borderTopWidth, normalizedOutRect.bottom),
            Math.max(normalizedOutRect.right - borderRightWidth, normalizedOutRect.left),
            Math.max(normalizedOutRect.bottom - borderBottomWidth, normalizedOutRect.top));

    if (normalizedInnerRect.right < normalizedInnerRect.left) {
      normalizedInnerRect.right = normalizedInnerRect.left;
    }
    if (normalizedInnerRect.bottom < normalizedInnerRect.top) {
      normalizedInnerRect.bottom = normalizedInnerRect.top;
    }

    return new BorderBoxes(createRoundedRectangle(normalizedOutRect, outBox),
        createRoundedRectangle(normalizedInnerRect, innerBox));
  }

  private void normalizeClipRect(RectF rect) {
    View hostLayer = getHostLayer();
    if (hostLayer instanceof AndroidScrollView) {
      rect.offset(hostLayer.getScrollX(), hostLayer.getScrollY());
    }
    if (!shouldNormalizeRootGeometry()) {
      return;
    }
    if (!rect.intersect(getHostBoundsRectF(true))) {
      rect.setEmpty();
    }
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
          // Begin fragment: id, type, x, y, width, height (2 ints, 4 floats)
          if (intParamCount >= 2) {
            nextContentInt(); // skip id
            nextContentInt(); // skip type
          }
          if (floatParamCount >= 4) {
            float x = nextContentFloat();
            float y = nextContentFloat();
            nextContentFloat(); // unused width
            nextContentFloat(); // unused height
            canvas.save();
            canvas.translate(x, y);
            mFragmentDepth++;
          }
          break;
        }

        case OP_END: {
          // End fragment - no parameters
          canvas.restore();
          if (mFragmentDepth > 0) {
            mFragmentDepth--;
          }
          break; // End of this sub view's content
        }

        case OP_FILL: {
          mPaint.reset();
          // Fill: color (1 int), clip_index (1 int)
          int color = nextContentInt();
          int clipIndex = nextContentInt();
          mPaint.setColor(color);

          RoundedRectangle roundedRectangle = getNormalizedRoundedRectangle(clipIndex);
          if (roundedRectangle != null) {
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

            BorderBoxes borderBoxes = getNormalizedBorderBoxes(outBoxIndex, innerBoxIndex);
            drawRectangularBorders(canvas, mPaint, borderBoxes.outBox, borderBoxes.innerBox,
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
          normalizeClipRect(mReusableRectF);
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

    RoundedRectangle tilingBox = getNormalizedRoundedRectangle(tilingIndex);
    if (tilingBox == null) {
      return;
    }

    RoundedRectangle clipBox = getNormalizedRoundedRectangle(clipIndex);
    if (clipBox == null) {
      return;
    }

    // Cache tiling box RectF to avoid repeated calls
    final RectF tilingRect = tilingBox.getRectF();
    final float width = tilingRect.width();
    final float height = tilingRect.height();
    final float left = tilingRect.left;
    final float top = tilingRect.top;

    PointF center = mReusablePointF1;
    center.set(width / 2.f, height / 2.f);
    double radial = Math.toRadians(angle);
    float sin = (float) Math.sin(radial);
    float cos = (float) Math.cos(radial);
    float tan = (float) Math.tan(radial);

    PointF m = mReusablePointF2;
    if (sin >= 0 && cos >= 0) { // Bottom left to top right
      m.set(width, top);
    } else if (sin >= 0 && cos < 0) { // Top left to bottom right
      m.set(width, height);
    } else if (sin < 0 && cos < 0) { // Top right to bottom left
      m.set(left, height);
    } else { // Bottom right to top left
      m.set(0, 0);
    }

    // Reference logic from BackgroundLinearGradientLayer.java
    float tmp = (center.y - m.y - tan * center.x + tan * m.x);
    float endX = center.x + sin * tmp / (sin * tan + cos);
    float endY = center.y - tmp / (tan * tan + 1);
    float startX = 2 * center.x - endX;
    float startY = 2 * center.y - endY;

    mPaint.reset();
    mPaint.setAntiAlias(true);

    // Always use CLAMP - we handle tiling manually to support directional repeats
    mPaint.setShader(
        new LinearGradient(startX, startY, endX, endY, colors, stops, Shader.TileMode.CLAMP));

    // Cache RectF values to avoid repeated getRectF() calls
    final RectF clipRect = clipBox.getRectF();
    final float clipLeft = clipRect.left;
    final float clipTop = clipRect.top;
    final float clipRight = clipRect.right;
    final float clipBottom = clipRect.bottom;

    // Clip to the clip box bounds
    canvas.save();
    if (clipBox.hasBorderRadius()) {
      mReusablePath.reset();
      mReusablePath.addRoundRect(clipRect, clipBox.getBorderRadii(), Path.Direction.CW);
      canvas.clipPath(mReusablePath);
    } else {
      canvas.clipRect(clipRect);
    }

    // Manual repeat handling similar to LayerManager.java
    // Calculate the drawing bounds based on repeat mode
    if (repeatX == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT
        && repeatY == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT) {
      // No repeat - draw once at tiling box position
      canvas.save();
      canvas.translate(left, top);
      canvas.drawRect(0, 0, width, height, mPaint);
      canvas.restore();
    } else {
      // Calculate end coordinates for tiling
      final float endTileX = clipRight;
      final float endTileY = clipBottom;

      // Calculate start coordinates with proper tiling alignment
      float tileStartX = left;
      float tileStartY = top;

      // For REPEAT or REPEAT_X: adjust start to cover area left of tiling box
      if (repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT
          || repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT_X) {
        if (tileStartX > clipLeft) {
          tileStartX = tileStartX - ((int) Math.ceil((tileStartX - clipLeft) / width)) * width;
        }
      }

      // For REPEAT or REPEAT_Y: adjust start to cover area above tiling box
      if (repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT
          || repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT_Y) {
        if (tileStartY > clipTop) {
          tileStartY = tileStartY - ((int) Math.ceil((tileStartY - clipTop) / height)) * height;
        }
      }

      // Tile the gradient - minimize canvas save/restore by using translate
      final int saveCount = canvas.save();
      // Round to pixel boundaries to prevent subpixel gaps between tiles
      final float pixelAlignedStartX = Math.round(tileStartX);
      final float pixelAlignedStartY = Math.round(tileStartY);
      final float pixelAlignedWidth = Math.round(width);
      final float pixelAlignedHeight = Math.round(height);
      canvas.translate(pixelAlignedStartX, pixelAlignedStartY);
      for (float x = pixelAlignedStartX; x < endTileX; x += pixelAlignedWidth) {
        final int rowSaveCount = canvas.save();
        for (float y = pixelAlignedStartY; y < endTileY; y += pixelAlignedHeight) {
          canvas.drawRect(0, 0, pixelAlignedWidth, pixelAlignedHeight, mPaint);
          canvas.translate(0, pixelAlignedHeight);

          // Break after first iteration if no repeat on Y
          if (repeatY == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT) {
            break;
          }
        }
        canvas.restoreToCount(rowSaveCount);
        canvas.translate(pixelAlignedWidth, 0);

        // Break after first iteration if no repeat on X
        if (repeatX == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT) {
          break;
        }
      }
      canvas.restoreToCount(saveCount);
    }
    canvas.restore();

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
  void drawRectangularBorders(Canvas canvas, Paint paint, RoundedRectangle outBox,
      RoundedRectangle innerBox, int[] borderColors, BorderStyle[] borderStyles) {
    if (outBox == null || innerBox == null) {
      LLog.e(TAG, "drawRectangularBorders failed since outBox or innerBox is null.");
      return;
    }

    BorderDrawingUtil.drawBorders(canvas, paint, outBox, innerBox, borderColors, borderStyles);
  }

  void drawRectangularBorders(Canvas canvas, Paint paint, int outBoxIndex, int innerBoxIndex,
      int[] borderColors, BorderStyle[] borderStyles) {
    BorderBoxes borderBoxes = getNormalizedBorderBoxes(outBoxIndex, innerBoxIndex);
    drawRectangularBorders(
        canvas, paint, borderBoxes.outBox, borderBoxes.innerBox, borderColors, borderStyles);
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

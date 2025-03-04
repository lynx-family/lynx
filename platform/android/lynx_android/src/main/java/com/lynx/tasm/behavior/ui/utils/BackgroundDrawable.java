/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * <p>
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Outline;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PathEffect;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.drawable.Drawable;
import android.os.Build;
import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.background.BackgroundLayerManager;
import java.util.HashMap;
import java.util.Map;

/**
 * A subclass of {@link Drawable} used for background of LynxBaseUI.
 * It supports drawing background color and borders (including rounded borders)
 * by providing a react friendly API (setter for each of those properties).
 *
 * <p>
 * The implementation tries to allocate as few objects as possible depending on
 * which properties are set. E.g. for views with rounded background/borders we
 * allocate {@code
 * mInnerClipPathForBorderRadius} and {@code innerClipRect}.
 * In case when view have a rectangular borders we allocate
 * {@code mBorderWidthResult} and similar. When only background color is set we
 * won't allocate any extra/unnecessary objects.
 */
public class BackgroundDrawable extends LayerDrawable<BackgroundLayerManager> {
  private static final int DEFAULT_BORDER_COLOR = Color.BLACK;
  private static final int DEFAULT_BORDER_RGB = 0x00FFFFFF & DEFAULT_BORDER_COLOR;
  private static final int DEFAULT_BORDER_ALPHA = (0xFF000000 & DEFAULT_BORDER_COLOR) >>> 24;
  // ~0 == 0xFFFFFFFF, all bits set to 1.
  private static final int ALL_BITS_SET = ~0;
  // 0 == 0x00000000, all bits set to 0.
  private static final int ALL_BITS_UNSET = 0;
  private boolean mPaddingWidthChanged = false;
  private boolean mBorderWidthChanged = false;

  public static class RoundRectPath {
    public RectF rect;
    public float[] radius;
    public Path path;
    public boolean allCornersWithSameRadius;

    static enum Pos {
      CENTER,
      INNER2,
      OUTER2,
      INNER3,
      OUTER3;

      public float getOffset() {
        switch (this) {
          case CENTER:
            return 0.5f;
          case INNER2:
            return 0.75f;
          case OUTER2:
            return 0.25f;
          case INNER3:
            return 5.0f / 6.0f;
          case OUTER3:
            return 1.0f / 6.0f;
        }
        return 0;
      }
    }

    public void updateValue(
        Rect bounds, float[] borderRadius, RectF borderWidth, float mul, boolean center) {
      if (rect == null) {
        rect = new RectF();
      }
      rect.left = bounds.left + borderWidth.left * mul;
      rect.top = bounds.top + borderWidth.top * mul;
      rect.right = bounds.right - borderWidth.right * mul;
      rect.bottom = bounds.bottom - borderWidth.bottom * mul;

      radius = center ? newCenterBorderRadius(borderRadius, borderWidth, mul)
                      : newBorderRadius(borderRadius, borderWidth, mul);

      allCornersWithSameRadius = checkAllCornersWithSameRadius(radius);

      if (path == null) {
        path = new Path();
      } else {
        path.reset();
      }
      path.addRoundRect(rect, radius, Path.Direction.CW);
    }

    public static float[] newBorderRadius(float[] borderRadius, RectF borderWidth, float mul) {
      return new float[] {Math.max(borderRadius[0] - borderWidth.left * mul, 0),
          Math.max(borderRadius[1] - borderWidth.top * mul, 0),
          Math.max(borderRadius[2] - borderWidth.right * mul, 0),
          Math.max(borderRadius[3] - borderWidth.top * mul, 0),
          Math.max(borderRadius[4] - borderWidth.right * mul, 0),
          Math.max(borderRadius[5] - borderWidth.bottom * mul, 0),
          Math.max(borderRadius[6] - borderWidth.left * mul, 0),
          Math.max(borderRadius[7] - borderWidth.bottom * mul, 0)};
    }

    public static float[] newCenterBorderRadius(
        float[] borderRadius, RectF borderWidth, float mul) {
      return new float[] {
          Math.max(borderRadius[0] - borderWidth.left * mul,
              (borderWidth.left > 0.0f) ? (borderRadius[0] / borderWidth.left) : 0.0f),
          Math.max(borderRadius[1] - borderWidth.top * mul,
              (borderWidth.top > 0.0f) ? (borderRadius[1] / borderWidth.top) : 0.0f),
          Math.max(borderRadius[2] - borderWidth.right * mul,
              (borderWidth.right > 0.0f) ? (borderRadius[2] / borderWidth.right) : 0.0f),
          Math.max(borderRadius[3] - borderWidth.top * mul,
              (borderWidth.top > 0.0f) ? (borderRadius[3] / borderWidth.top) : 0.0f),
          Math.max(borderRadius[4] - borderWidth.right * mul,
              (borderWidth.right > 0.0f) ? (borderRadius[4] / borderWidth.right) : 0.0f),
          Math.max(borderRadius[5] - borderWidth.bottom * mul,
              (borderWidth.bottom > 0.0f) ? (borderRadius[5] / borderWidth.bottom) : 0.0f),
          Math.max(borderRadius[6] - borderWidth.left * mul,
              (borderWidth.left > 0.0f) ? (borderRadius[6] / borderWidth.left) : 0.0f),
          Math.max(borderRadius[7] - borderWidth.bottom * mul,
              (borderWidth.bottom > 0.0f) ? (borderRadius[7] / borderWidth.bottom) : 0.0f)};
    }

    public static boolean checkAllCornersWithSameRadius(float[] radius) {
      final float threshold = (float) 1e-4;
      for (int i = 2; i <= 6; i += 2) {
        float diffX = radius[i] - radius[0];
        if (diffX > threshold || diffX < -threshold) {
          return false;
        }
        float diffY = radius[i + 1] - radius[1];
        if (diffY > threshold || diffY < -threshold) {
          return false;
        }
      }
      return true;
    }

    public void drawToCanvas(Canvas canvas, Paint paint) {
      if (allCornersWithSameRadius) {
        canvas.drawRoundRect(rect, radius[0], radius[1], paint);
      } else {
        canvas.drawPath(path, paint);
      }
    }
  }

  private @Nullable Spacing mBorderRGB;
  private @Nullable Spacing mBorderAlpha;
  private @Nullable BorderStyle[] mBorderStyle;

  /* Used for rounded border and rounded background */
  private @Nullable PathEffect mPathEffectForBorderStyle;

  private @Nullable RoundRectPath mInnerClipPathForBorderRadius;
  private @Nullable RoundRectPath mOuterClipPathForBorderRadius;
  private @Nullable Map<RoundRectPath.Pos, RoundRectPath> mPathCache;

  private @Nullable Path mPathForBorderRadiusOutline;
  private @Nullable Path mPathForBorder;

  private @Nullable PointF mInnerTopLeftCorner;
  private @Nullable PointF mInnerTopRightCorner;
  private @Nullable PointF mInnerBottomRightCorner;
  private @Nullable PointF mInnerBottomLeftCorner;

  private boolean mNeedUpdatePathForBorderRadius = true;

  /* Used by all types of background and for drawing borders */
  private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private int mColor = Color.TRANSPARENT;
  private int mAlpha = 255;

  private @Nullable BorderRadius mBorderCornerRadii;

  private UIShadowProxy.InsetDrawer mBoxShadowInsetDrawer;

  public enum BorderRadiusLocation {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT,
    TOP_START,
    TOP_END,
    BOTTOM_START,
    BOTTOM_END
  }

  private RoundRectPath getPathFromCache(RoundRectPath.Pos pos) {
    if (pos == null || mPathCache == null) {
      return null;
    }
    return mPathCache.get(pos);
  }

  private void updateCachePath(
      RoundRectPath.Pos pos, Rect bounds, float[] borderRadius, RectF borderWidth) {
    if (pos == null || borderRadius == null) {
      return;
    }

    RoundRectPath path = null;
    try {
      if (mPathCache != null) {
        path = mPathCache.get(pos);
      } else {
        mPathCache = new HashMap<>();
      }
      if (path == null) {
        path = new RoundRectPath();
        mPathCache.put(pos, path);
      }
      path.updateValue(bounds, borderRadius, borderWidth, pos.getOffset(), true);
    } catch (Exception e) {
      LLog.e("BackgroundDrawable", "updateCachePath exception:" + e.toString());
    }
  }

  public BackgroundDrawable(LynxContext lynxContext, float curFontSize) {
    super(lynxContext, curFontSize);
  }

  @Override
  protected BackgroundLayerManager createLayerManager() {
    return new BackgroundLayerManager(mContext, this, mCurFontSize);
  }

  @Override
  public void draw(Canvas canvas) {
    boolean needUpdateContentBox = mPaddingWidthChanged || mBorderWidthChanged;
    if (needUpdateContentBox) {
      updateContentBox();
      mPaddingWidthChanged = mBorderWidthChanged = false;
    }
    drawBackGround(canvas);

    if (mBorderCornerRadii == null || !mBorderCornerRadii.hasRoundedBorders()) {
      drawRectangularBorders(canvas);
    } else {
      drawRoundedBorders(canvas);
    }

    if (mBoxShadowInsetDrawer != null) {
      mBoxShadowInsetDrawer.draw(canvas);
    }
  }

  private void drawRectangularRect(Canvas canvas) {
    int clip = mLayerManager.getLayerClip();
    if (clip == StyleConstants.BACKGROUND_CLIP_BORDER_BOX) {
      canvas.drawRect(getBounds(), mPaint);
    } else if (clip == StyleConstants.BACKGROUND_CLIP_PADDING_BOX) {
      canvas.drawRect(mPaddingBox, mPaint);
    } else { // BACKGROUND_CLIP_CONTENT_BOX
      canvas.drawRect(mContentBox, mPaint);
    }
  }

  private void drawRoundedRect(Canvas canvas) {
    int clip = mLayerManager.getLayerClip();
    if (clip == StyleConstants.BACKGROUND_CLIP_BORDER_BOX) {
      // If border has transparent color or style, use outer path
      if (hasTransparentBorder() && mOuterClipPathForBorderRadius != null) {
        mOuterClipPathForBorderRadius.drawToCanvas(canvas, mPaint);
      } else if (mInnerClipPathForBorderRadius != null) {
        // Use inner path because of anti-aliasing issues
        mInnerClipPathForBorderRadius.drawToCanvas(canvas, mPaint);
      }
    } else if (clip == StyleConstants.BACKGROUND_CLIP_PADDING_BOX
        && mInnerClipPathForBorderRadius != null) {
      mInnerClipPathForBorderRadius.drawToCanvas(canvas, mPaint);
    } else {
      canvas.drawRect(mContentBox, mPaint);
    }
  }

  public void drawBackGround(Canvas canvas) {
    int useColor = ColorUtil.multiplyColorAlpha(mColor, mAlpha);
    if (Color.alpha(useColor) != 0) {
      mPaint.setColor(useColor);
      mPaint.setStyle(Paint.Style.FILL);
      if (mBorderCornerRadii == null || !mBorderCornerRadii.hasRoundedBorders()) {
        drawRectangularRect(canvas);
      } else if (updatePath()) {
        drawRoundedRect(canvas);
      }
    }

    if (mLayerManager.hasImageLayers()) {
      canvas.save();

      RectF borderRect = new RectF(getBounds());
      RectF paddingRect = new RectF(mPaddingBox);
      RectF contentRect = new RectF(mContentBox);

      if (mBorderCornerRadii != null) {
        updatePath();
      }

      RectF clipBox = borderRect;
      Path outerPath = null;
      Path innerPath = null;
      if (mOuterClipPathForBorderRadius != null) {
        outerPath = mOuterClipPathForBorderRadius.path;
      }

      if (mInnerClipPathForBorderRadius != null) {
        innerPath = mInnerClipPathForBorderRadius.path;
      }

      mLayerManager.draw(canvas, borderRect, paddingRect, contentRect, clipBox, outerPath,
          innerPath, mBorderWidth != null);

      canvas.restore();
    }
  }

  @Override
  protected void onBoundsChange(Rect bounds) {
    mNeedUpdatePathForBorderRadius = true;
    super.onBoundsChange(bounds);
  }

  @Override
  public void setAlpha(int alpha) {
    if (alpha != mAlpha) {
      mAlpha = alpha;
      invalidateSelf();
    }
  }

  @Override
  public int getAlpha() {
    return mAlpha;
  }

  @Override
  public void setColorFilter(ColorFilter cf) {
    // do nothing
  }

  @Override
  public int getOpacity() {
    return ColorUtil.getOpacityFromColor(ColorUtil.multiplyColorAlpha(mColor, mAlpha));
  }

  /*
   * Android's elevation implementation requires this to be implemented to know
   * where to draw the shadow.
   */
  @Override
  public void getOutline(Outline outline) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
      super.getOutline(outline);
      return;
    }
    if (mBorderCornerRadii != null && updatePath()) {
      if (mPathForBorderRadiusOutline != null) {
        outline.setConvexPath(mPathForBorderRadiusOutline);
        return;
      }
    }
    outline.setRect(getBounds());
  }

  @Override
  public boolean setBorderWidth(int position, float width) {
    boolean update = super.setBorderWidth(position, width);
    if (update) {
      mNeedUpdatePathForBorderRadius = true;
      mBorderWidthChanged = true;
    }
    return update;
  }

  @Override
  public boolean setPaddingWidth(float top, float right, float bottom, float left) {
    boolean update = super.setPaddingWidth(top, right, bottom, left);
    if (update) {
      mNeedUpdatePathForBorderRadius = true;
      mPaddingWidthChanged = true;
    }
    return update;
  }

  public void setBorderColor(int position, float rgb, float alpha) {
    this.setBorderRGB(position, rgb);
    this.setBorderAlpha(position, alpha);
  }

  private void setBorderRGB(int position, float rgb) {
    // set RGB component
    if (mBorderRGB == null) {
      mBorderRGB = new Spacing(DEFAULT_BORDER_RGB);
    }
    mBorderRGB.set(position, rgb);
  }

  private void setBorderAlpha(int position, float alpha) {
    // set Alpha component
    if (mBorderAlpha == null) {
      mBorderAlpha = new Spacing(DEFAULT_BORDER_ALPHA);
    }
    mBorderAlpha.set(position, alpha);
  }

  public BorderStyle getBorderStyle(int position) {
    if (position > Spacing.ALL || position < 0 || mBorderStyle == null)
      return null;
    if (mBorderStyle[position] != null) {
      return mBorderStyle[position];
    }
    return mBorderStyle[Spacing.ALL];
  }

  public void setBorderStyle(int position, int style) {
    if (position > Spacing.ALL || position < 0)
      return;

    if (mBorderStyle == null) {
      mBorderStyle = new BorderStyle[Spacing.ALL + 1];
    }

    try {
      BorderStyle borderStyle = BorderStyle.parse(style);
      if (mBorderStyle[position] != borderStyle) {
        mBorderStyle[position] = borderStyle;
        invalidateSelf();
      }
    } catch (Throwable e) {
    }
  }

  public BorderRadius getBorderRadius() {
    return mBorderCornerRadii;
  }

  public void setBorderRadiusCorner(int position, BorderRadius.Corner radius) {
    if (position <= 0 || position > 8) {
      // do not use position all
      return;
    }

    if (mBorderCornerRadii == null) {
      mBorderCornerRadii = new BorderRadius();
      updateCornerRadii();
    } else {
      mBorderCornerRadii.clearCache();
    }

    if (mBorderCornerRadii.setCorner(position - 1, radius)) {
      mNeedUpdatePathForBorderRadius = true;
      invalidateSelf();
    }
  }

  private void updateCornerRadii() {
    if (mBorderCornerRadii != null) {
      final Rect bounds = getBounds();
      mBorderCornerRadii.updateSize(bounds.width(), bounds.height());
    }
  }

  @Keep
  public void setColor(int color) {
    mColor = color;
    invalidateSelf();
  }

  /** Similar to Drawable.getLayoutDirection, but available in APIs < 23. */

  public int getColor() {
    return mColor;
  }

  private void drawRoundedBorders(Canvas canvas) {
    if (!updatePath()) {
      return;
    }

    canvas.save();

    final RectF borderWidth = getDirectionAwareBorderInsets();
    final int borderLeft = calcBorderMeasureWidth(borderWidth.left);
    final int borderTop = calcBorderMeasureWidth(borderWidth.top);
    final int borderRight = calcBorderMeasureWidth(borderWidth.right);
    final int borderBottom = calcBorderMeasureWidth(borderWidth.bottom);

    if (borderTop > 0 || borderBottom > 0 || borderLeft > 0 || borderRight > 0) {
      // If it's a full and even border draw inner rect path with stroke
      int borderColor = getBorderColor(Spacing.ALL);

      int borderColorLeft = getBorderColor(Spacing.LEFT);
      final boolean isBorderColorSame = (borderColorLeft == getBorderColor(Spacing.RIGHT)
          && borderColorLeft == getBorderColor(Spacing.TOP)
          && borderColorLeft == getBorderColor(Spacing.BOTTOM));
      if (isBorderColorSame) {
        borderColor = borderColorLeft;
      }

      final boolean isBorderWidthSame =
          (borderTop == borderLeft && borderBottom == borderLeft && borderRight == borderLeft);

      if (isBorderWidthSame && isBorderColorSame && toDrawBorderUseSameStyle() && borderLeft > 0) {
        strokeCenterDrawPath(canvas, Spacing.TOP, borderColor, borderLeft, borderLeft);
      }
      // In the case of uneven border widths/colors draw quadrilateral in each
      // direction
      else {
        int colorLeft = getBorderColor(Spacing.LEFT);
        int colorTop = getBorderColor(Spacing.TOP);
        int colorRight = getBorderColor(Spacing.RIGHT);
        int colorBottom = getBorderColor(Spacing.BOTTOM);

        final RectF outerClipRect = mOuterClipPathForBorderRadius.rect;
        final float left = outerClipRect.left;
        final float right = outerClipRect.right;
        final float top = outerClipRect.top;
        final float bottom = outerClipRect.bottom;

        if (borderTop > 0 && Color.alpha(colorTop) != 0) {
          final float x1 = left;
          final float y1 = top;
          final float x2 = mInnerTopLeftCorner.x;
          final float y2 = mInnerTopLeftCorner.y;
          final float x3 = mInnerTopRightCorner.x;
          final float y3 = mInnerTopRightCorner.y;
          final float x4 = right;
          final float y4 = top;
          boolean toClip = false;
          float w = borderWidth.top;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderWidth.left, borderWidth.right));
            toClip = w - Math.min(borderWidth.left, borderWidth.right) >= 2;
          }
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip);
          strokeCenterDrawPath(canvas, Spacing.TOP, colorTop, borderWidth.top, w);
          canvas.restore();
        }

        if (borderRight > 0 && Color.alpha(colorRight) != 0) {
          final float x1 = right;
          final float y1 = top;
          final float x2 = mInnerTopRightCorner.x;
          final float y2 = mInnerTopRightCorner.y;
          final float x3 = mInnerBottomRightCorner.x;
          final float y3 = mInnerBottomRightCorner.y;
          final float x4 = right;
          final float y4 = bottom;
          boolean toClip = false;
          float w = borderWidth.right;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderWidth.top, borderWidth.bottom));
            toClip = w - Math.min(borderWidth.top, borderWidth.bottom) >= 2;
          }
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip);
          strokeCenterDrawPath(canvas, Spacing.RIGHT, colorRight, borderWidth.right, w);
          canvas.restore();
        }

        if (borderBottom > 0 && Color.alpha(colorBottom) != 0) {
          final float x1 = left;
          final float y1 = bottom;
          final float x2 = mInnerBottomLeftCorner.x;
          final float y2 = mInnerBottomLeftCorner.y;
          final float x3 = mInnerBottomRightCorner.x;
          final float y3 = mInnerBottomRightCorner.y;
          final float x4 = right;
          final float y4 = bottom;
          boolean toClip = false;
          float w = borderWidth.bottom;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderWidth.left, borderWidth.right));
            toClip = w - Math.min(borderWidth.left, borderWidth.right) >= 2;
          }
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip);
          strokeCenterDrawPath(canvas, Spacing.BOTTOM, colorBottom, borderWidth.bottom, w);
          canvas.restore();
        }

        if (borderLeft > 0 && Color.alpha(colorLeft) != 0) {
          final float x1 = left;
          final float y1 = top;
          final float x2 = mInnerTopLeftCorner.x;
          final float y2 = mInnerTopLeftCorner.y;
          final float x3 = mInnerBottomLeftCorner.x;
          final float y3 = mInnerBottomLeftCorner.y;
          final float x4 = left;
          final float y4 = bottom;
          boolean toClip = false;
          float w = borderWidth.left;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderWidth.top, borderWidth.bottom));
            toClip = w - Math.min(borderWidth.top, borderWidth.bottom) >= 2;
          }
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip);
          strokeCenterDrawPath(canvas, Spacing.LEFT, colorLeft, borderWidth.left, w);
          canvas.restore();
        }
      }
    }

    canvas.restore();
  }

  private boolean updatePath() {
    if (mBorderCornerRadii == null) {
      return false;
    }

    if (!mNeedUpdatePathForBorderRadius) {
      return true;
    }

    final Rect bounds = getBounds();
    if (bounds.width() == 0 || bounds.height() == 0) {
      return false;
    }

    mNeedUpdatePathForBorderRadius = false;

    final RectF borderWidth = getDirectionAwareBorderInsets();
    updateCornerRadii();

    final float[] radiusArray = (mBorderCornerRadii != null ? mBorderCornerRadii.getArray() : null);

    if (mInnerClipPathForBorderRadius == null) {
      mInnerClipPathForBorderRadius = new RoundRectPath();
    }
    mInnerClipPathForBorderRadius.updateValue(bounds, radiusArray, borderWidth, 1.0f, false);

    if (mOuterClipPathForBorderRadius == null) {
      mOuterClipPathForBorderRadius = new RoundRectPath();
    }
    mOuterClipPathForBorderRadius.updateValue(bounds, radiusArray, borderWidth, 0.0f, false);

    if (mBorderCornerRadii != null && mBorderCornerRadii.hasRoundedBorders()) {
      updateCachePath(RoundRectPath.Pos.CENTER, bounds, radiusArray, borderWidth);
      if (mBorderStyle != null) {
        boolean hasDouble = false, hasGrooveOrRidge = false;
        for (int i = 0; i <= Spacing.ALL; ++i) {
          BorderStyle style = mBorderStyle[i];
          if (style != null) {
            if (style == BorderStyle.DOUBLE) {
              hasDouble = true;
            } else if (style == BorderStyle.GROOVE || style == BorderStyle.RIDGE) {
              hasGrooveOrRidge = true;
            }
          }
        }
        if (hasDouble) {
          updateCachePath(RoundRectPath.Pos.INNER3, bounds, radiusArray, borderWidth);
          updateCachePath(RoundRectPath.Pos.OUTER3, bounds, radiusArray, borderWidth);
        }
        if (hasGrooveOrRidge) {
          updateCachePath(RoundRectPath.Pos.INNER2, bounds, radiusArray, borderWidth);
          updateCachePath(RoundRectPath.Pos.OUTER2, bounds, radiusArray, borderWidth);
        }
      }
    }

    if (mPathForBorderRadiusOutline == null) {
      mPathForBorderRadiusOutline = new Path();
    } else {
      mPathForBorderRadiusOutline.reset();
    }
    mPathForBorderRadiusOutline.addRoundRect(new RectF(bounds),
        RoundRectPath.newBorderRadius(radiusArray, borderWidth, -0.5f), Path.Direction.CW);

    roundMultiColoredBorderAlgorithm();

    return true;
  }

  private void roundMultiColoredBorderAlgorithm() {
    /**
     * Rounded Multi-Colored Border Algorithm:
     *
     * <p>
     * Let O (for outer) = (top, left, bottom, right) be the rectangle that
     * represents the size and position of a view V. Since the box-sizing of all
     * React Native views is border-box, any border of V will render inside O.
     *
     * <p>
     * Let BorderWidth = (borderTop, borderLeft, borderBottom, borderRight).
     *
     * <p>
     * Let I (for inner) = O - BorderWidth.
     *
     * <p>
     * Then, remembering that O and I are rectangles and that I is inside O, O - I
     * gives us the border of V. Therefore, we can use canvas.clipPath to draw V's
     * border.
     *
     * <p>
     * canvas.clipPath(O, Region.OP.INTERSECT);
     *
     * <p>
     * canvas.clipPath(I, Region.OP.DIFFERENCE);
     *
     * <p>
     * canvas.drawRect(O, paint);
     *
     * <p>
     * This lets us draw non-rounded single-color borders.
     *
     * <p>
     * To extend this algorithm to rounded single-color borders, we:
     *
     * <p>
     * 1. Curve the corners of O by the (border radii of V) using Path#addRoundRect.
     *
     * <p>
     * 2. Curve the corners of I by (border radii of V - border widths of V) using
     * Path#addRoundRect.
     *
     * <p>
     * Let O' = curve(O, border radii of V).
     *
     * <p>
     * Let I' = curve(I, border radii of V - border widths of V)
     *
     * <p>
     * The rationale behind this decision is the (first sentence of the) following
     * section in the CSS Backgrounds and Borders Module Level 3:
     * https://www.w3.org/TR/css3-background/#the-border-radius.
     *
     * <p>
     * After both O and I have been curved, we can execute the following lines once
     * again to render curved single-color borders:
     *
     * <p>
     * canvas.clipPath(O, Region.OP.INTERSECT);
     *
     * <p>
     * canvas.clipPath(I, Region.OP.DIFFERENCE);
     *
     * <p>
     * canvas.drawRect(O, paint);
     *
     * <p>
     * To extend this algorithm to rendering multi-colored rounded borders, we
     * render each side of the border as its own quadrilateral. Suppose that we were
     * handling the case where all the border radii are 0. Then, the four
     * quadrilaterals would be:
     *
     * <p>
     * Left: (O.left, O.top), (I.left, I.top), (I.left, I.bottom), (O.left,
     * O.bottom)
     *
     * <p>
     * Top: (O.left, O.top), (I.left, I.top), (I.right, I.top), (O.right, O.top)
     *
     * <p>
     * Right: (O.right, O.top), (I.right, I.top), (I.right, I.bottom), (O.right,
     * O.bottom)
     *
     * <p>
     * Bottom: (O.right, O.bottom), (I.right, I.bottom), (I.left, I.bottom),
     * (O.left, O.bottom)
     *
     * <p>
     * Now, lets consider what happens when we render a rounded border (radii != 0).
     * For the sake of simplicity, let's focus on the top edge of the Left border:
     *
     * <p>
     * Let borderTopLeftRadius = 5. Let borderLeftWidth = 1. Let borderTopWidth = 2.
     *
     * <p>
     * We know that O is curved by the ellipse E_O (a = 5, b = 5). We know that I is
     * curved by the ellipse E_I (a = 5 - 1, b = 5 - 2).
     *
     * <p>
     * Since we have clipping, it should be safe to set the top-left point of the
     * Left quadrilateral's top edge to (O.left, O.top).
     *
     * <p>
     * But, what should the top-right point be?
     *
     * <p>
     * The fact that the border is curved shouldn't change the slope (nor the
     * position) of the line connecting the top-left and top-right points of the
     * Left quadrilateral's top edge. Therefore, The top-right point should lie
     * somewhere on the line L = (1 - a) * (O.left, O.top) + a * (I.left, I.top).
     *
     * <p>
     * a != 0, because then the top-left and top-right points would be the same and
     * borderLeftWidth = 1. a != 1, because then the top-right point would not touch
     * an edge of the ellipse E_I. We want the top-right point to touch an edge of
     * the inner ellipse because the border curves with E_I on the top-left corner
     * of V.
     *
     * <p>
     * Therefore, it must be the case that a > 1. Two natural locations of the
     * top-right point exist: 1. The first intersection of L with E_I. 2. The second
     * intersection of L with E_I.
     *
     * <p>
     * We choose the top-right point of the top edge of the Left quadrilateral to be
     * an arbitrary intersection of L with E_I.
     */

    final RectF innerClipRect = mInnerClipPathForBorderRadius.rect;
    final RectF outerClipRect = mOuterClipPathForBorderRadius.rect;
    final float[] innerRadius = mInnerClipPathForBorderRadius.radius;

    if (mInnerTopLeftCorner == null) {
      mInnerTopLeftCorner = new PointF();
    }

    /** Compute mInnerTopLeftCorner */
    mInnerTopLeftCorner.x = mInnerClipPathForBorderRadius.rect.left;
    mInnerTopLeftCorner.y = mInnerClipPathForBorderRadius.rect.top;

    getEllipseIntersectionWithLine(
        // Ellipse Bounds
        innerClipRect.left, innerClipRect.top, innerClipRect.left + 2 * innerRadius[0],
        innerClipRect.top + 2 * innerRadius[1],

        // Line Start
        outerClipRect.left, outerClipRect.top,

        // Line End
        innerClipRect.left, innerClipRect.top,

        // Result
        mInnerTopLeftCorner);

    /** Compute mInnerBottomLeftCorner */
    if (mInnerBottomLeftCorner == null) {
      mInnerBottomLeftCorner = new PointF();
    }

    mInnerBottomLeftCorner.x = innerClipRect.left;
    mInnerBottomLeftCorner.y = innerClipRect.bottom;

    getEllipseIntersectionWithLine(
        // Ellipse Bounds
        innerClipRect.left, innerClipRect.bottom - 2 * innerRadius[6],
        innerClipRect.left + 2 * innerRadius[7], innerClipRect.bottom,

        // Line Start
        outerClipRect.left, outerClipRect.bottom,

        // Line End
        innerClipRect.left, innerClipRect.bottom,

        // Result
        mInnerBottomLeftCorner);

    /** Compute mInnerTopRightCorner */
    if (mInnerTopRightCorner == null) {
      mInnerTopRightCorner = new PointF();
    }

    mInnerTopRightCorner.x = innerClipRect.right;
    mInnerTopRightCorner.y = innerClipRect.top;

    getEllipseIntersectionWithLine(
        // Ellipse Bounds
        innerClipRect.right - 2 * innerRadius[2], innerClipRect.top, innerClipRect.right,
        innerClipRect.top + 2 * innerRadius[3],

        // Line Start
        outerClipRect.right, outerClipRect.top,

        // Line End
        innerClipRect.right, innerClipRect.top,

        // Result
        mInnerTopRightCorner);

    /** Compute mInnerBottomRightCorner */
    if (mInnerBottomRightCorner == null) {
      mInnerBottomRightCorner = new PointF();
    }

    mInnerBottomRightCorner.x = innerClipRect.right;
    mInnerBottomRightCorner.y = innerClipRect.bottom;

    getEllipseIntersectionWithLine(
        // Ellipse Bounds
        innerClipRect.right - 2 * innerRadius[4], innerClipRect.bottom - 2 * innerRadius[5],
        innerClipRect.right, innerClipRect.bottom,

        // Line Start
        outerClipRect.right, outerClipRect.bottom,

        // Line End
        innerClipRect.right, innerClipRect.bottom,

        // Result
        mInnerBottomRightCorner);
  }

  private static void getEllipseIntersectionWithLine(double ellipseBoundsLeft,
      double ellipseBoundsTop, double ellipseBoundsRight, double ellipseBoundsBottom,
      double lineStartX, double lineStartY, double lineEndX, double lineEndY, PointF result) {
    final double ellipseCenterX = (ellipseBoundsLeft + ellipseBoundsRight) / 2;
    final double ellipseCenterY = (ellipseBoundsTop + ellipseBoundsBottom) / 2;

    /**
     * Step 1:
     *
     * Translate the line so that the ellipse is at the origin.
     *
     * Why? It makes the math easier by changing the ellipse equation from ((x -
     * ellipseCenterX)/a)^2 + ((y - ellipseCenterY)/b)^2 = 1 to (x/a)^2 + (y/b)^2 =
     * 1.
     */
    lineStartX -= ellipseCenterX;
    lineStartY -= ellipseCenterY;
    lineEndX -= ellipseCenterX;
    lineEndY -= ellipseCenterY;

    /**
     * Step 2:
     *
     * Ellipse equation: (x/a)^2 + (y/b)^2 = 1 Line equation: y = mx + c
     */
    final double a = Math.abs(ellipseBoundsRight - ellipseBoundsLeft) / 2;
    final double b = Math.abs(ellipseBoundsBottom - ellipseBoundsTop) / 2;
    final double m = (lineEndY - lineStartY) / (lineEndX - lineStartX);
    final double c = lineStartY - m * lineStartX; // Just a point on the line

    /**
     * Step 3:
     *
     * Substitute the Line equation into the Ellipse equation. Solve for x.
     * Eventually, you'll have to use the quadratic formula.
     *
     * Quadratic formula: Ax^2 + Bx + C = 0
     */
    final double A = (b * b + a * a * m * m);
    final double B = 2 * a * a * c * m;
    final double C = (a * a * (c * c - b * b));

    /**
     * Step 4:
     *
     * Apply Quadratic formula. D = determinant / 2A
     */
    final double D = Math.sqrt(-C / A + Math.pow(B / (2 * A), 2));
    final double x2 = -B / (2 * A) - D;
    final double y2 = m * x2 + c;

    /**
     * Step 5:
     *
     * Undo the space transformation in Step 5.
     */
    final double x = x2 + ellipseCenterX;
    final double y = y2 + ellipseCenterY;

    if (!Double.isNaN(x) && !Double.isNaN(y)) {
      result.x = (float) x;
      result.y = (float) y;
    }
  }

  /**
   * Quickly determine if all the set border colors are equal. Bitwise AND all the
   * set colors together, then OR them all together. If the AND and the OR are the
   * same, then the colors are compatible, so return this color.
   * <p>
   * Used to avoid expensive path creation and expensive calls to canvas.drawPath
   *
   * @return A compatible border color, or zero if the border colors are not
   * compatible.
   */
  private static int fastBorderCompatibleColorOrZero(int borderLeft, int borderTop, int borderRight,
      int borderBottom, int colorLeft, int colorTop, int colorRight, int colorBottom) {
    int andSmear = (borderLeft > 0 ? colorLeft : ALL_BITS_SET)
        & (borderTop > 0 ? colorTop : ALL_BITS_SET) & (borderRight > 0 ? colorRight : ALL_BITS_SET)
        & (borderBottom > 0 ? colorBottom : ALL_BITS_SET);
    int orSmear = (borderLeft > 0 ? colorLeft : ALL_BITS_UNSET)
        | (borderTop > 0 ? colorTop : ALL_BITS_UNSET)
        | (borderRight > 0 ? colorRight : ALL_BITS_UNSET)
        | (borderBottom > 0 ? colorBottom : ALL_BITS_UNSET);
    return andSmear == orSmear ? andSmear : 0;
  }

  private boolean toDrawBorderUseSameStyle() {
    if (mBorderStyle == null)
      return true;

    final BorderStyle all = mBorderStyle[Spacing.ALL];
    final BorderStyle left =
        (mBorderStyle[Spacing.LEFT] != null ? mBorderStyle[Spacing.LEFT] : all);

    final BorderStyle right =
        (mBorderStyle[Spacing.RIGHT] != null ? mBorderStyle[Spacing.RIGHT] : all);
    if (right != left)
      return false;
    final BorderStyle top = (mBorderStyle[Spacing.TOP] != null ? mBorderStyle[Spacing.TOP] : all);
    if (top != left)
      return false;
    final BorderStyle bottom =
        (mBorderStyle[Spacing.BOTTOM] != null ? mBorderStyle[Spacing.BOTTOM] : all);
    if (bottom != left)
      return false;

    return (left == null || left.isSolidDashedOrDotted());
  }

  private static int darkenColor(int color) {
    return ((color & 0x00FEFEFE) >> 1) | (color & 0xFF000000);
  }
  //
  //  private void strokeBorderMoreLines(Canvas canvas, int borderPosition, float borderWidth,
  //      float borderOffset, int color0, int color1, float startX, float startY, float stopX,
  //      float stopY) {
  //    mPaint.setPathEffect(null);
  //    mPaint.setStrokeWidth(borderWidth);
  //
  //    for (int i = -1; i <= 1; i += 2) {
  //      float xOffset = 0, yOffset = 0;
  //      int color = 0;
  //      switch (borderPosition) {
  //        case Spacing.TOP:
  //          yOffset = borderOffset * i;
  //          color = (i == 1 ? color0 : color1);
  //          break;
  //        case Spacing.RIGHT:
  //          xOffset = -borderOffset * i;
  //          color = (i == 1 ? color1 : color0);
  //          break;
  //        case Spacing.BOTTOM:
  //          yOffset = -borderOffset * i;
  //          color = (i == 1 ? color1 : color0);
  //          break;
  //        case Spacing.LEFT:
  //          xOffset = borderOffset * i;
  //          color = (i == 1 ? color0 : color1);
  //          break;
  //      }
  //
  //      mPaint.setColor(ColorUtil.multiplyColorAlpha(color, mAlpha));
  //      canvas.drawLine(startX + xOffset, startY + yOffset, stopX + xOffset, stopY + yOffset,
  //      mPaint);
  //    }
  //  }

  private BorderStyle getBorderStyleWithDefaultSolid(int borderPosition) {
    BorderStyle borderStyle =
        (mBorderStyle == null ? null
                              : (mBorderStyle[borderPosition] != null ? mBorderStyle[borderPosition]
                                                                      : mBorderStyle[Spacing.ALL]));
    if (borderStyle == null) {
      if (mContext.getCssAlignWithLegacyW3c()) {
        borderStyle = BorderStyle.NONE;
      } else {
        borderStyle = BorderStyle.SOLID;
      }
    }
    return borderStyle;
  }

  /*
  private void strokeBorderLine(Canvas canvas, int borderPosition, int color, float startX,
      float startY, float stopX, float stopY, float borderLength, float borderWidth,
      BorderStyle externalStyle) {
    BorderStyle borderStyle = externalStyle;
    if (borderStyle == null) {
      borderStyle = (mBorderStyle == null
              ? null
              : (mBorderStyle[borderPosition] != null ? mBorderStyle[borderPosition]
                                                      : mBorderStyle[Spacing.ALL]));
    }
    if (borderStyle == null) {
      borderStyle = BorderStyle.SOLID;
    }

    mPathEffectForBorderStyle = null;
    switch (borderStyle) {
      case NONE:
      case HIDDEN:
        return;

      case DASHED:
      case DOTTED:
        mPathEffectForBorderStyle = borderStyle.getPathEffectAutoAdjust(borderWidth, borderLength);
        break;

      case SOLID:
        break;
      case INSET:
        if (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT) {
          color = darkenColor(color);
        }
        break;
      case OUTSET:
        if (borderPosition == Spacing.BOTTOM || borderPosition == Spacing.RIGHT) {
          color = darkenColor(color);
        }
        break;

      case DOUBLE:
        strokeBorderMoreLines(canvas, borderPosition, borderWidth / 3, borderWidth / 3, color,
            color, startX, startY, stopX, stopY);
        return;
      case GROOVE:
        strokeBorderMoreLines(canvas, borderPosition, borderWidth / 2, borderWidth / 4, color,
            darkenColor(color), startX, startY, stopX, stopY);
        return;
      case RIDGE:
        strokeBorderMoreLines(canvas, borderPosition, borderWidth / 2, borderWidth / 4,
            darkenColor(color), color, startX, startY, stopX, stopY);
        return;
    }

    mPaint.setColor(ColorUtil.multiplyColorAlpha(color, mAlpha));
    mPaint.setPathEffect(mPathEffectForBorderStyle);
    mPaint.setStrokeWidth(borderWidth);
    canvas.drawLine(startX, startY, stopX, stopY, mPaint);
    mPaint.setPathEffect(null);
  }*/

  private void strokeCenterDrawPathMoreLines(Canvas canvas, int borderPosition, float borderWidth,
      int color0, int color1, boolean isDoubleStyle) {
    mPaint.setPathEffect(null);
    mPaint.setStyle(Paint.Style.STROKE);
    mPaint.setStrokeWidth(borderWidth);

    final boolean isTopLeft = (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT);

    mPaint.setColor(ColorUtil.multiplyColorAlpha(isTopLeft ? color1 : color0, mAlpha));
    RoundRectPath centerPathOuter =
        getPathFromCache(isDoubleStyle ? RoundRectPath.Pos.OUTER3 : RoundRectPath.Pos.OUTER2);
    if (centerPathOuter != null) {
      centerPathOuter.drawToCanvas(canvas, mPaint);
    }

    mPaint.setColor(ColorUtil.multiplyColorAlpha(isTopLeft ? color0 : color1, mAlpha));
    RoundRectPath centerPathInner =
        getPathFromCache(isDoubleStyle ? RoundRectPath.Pos.INNER3 : RoundRectPath.Pos.INNER2);
    if (centerPathInner != null) {
      centerPathInner.drawToCanvas(canvas, mPaint);
    }
  }

  private void strokeCenterDrawPath(Canvas canvas, int borderPosition, int borderColor,
      float borderWidthForEffect, float borderWidthForStroke) {
    BorderStyle borderStyle =
        (mBorderStyle == null ? null
                              : (mBorderStyle[borderPosition] != null ? mBorderStyle[borderPosition]
                                                                      : mBorderStyle[Spacing.ALL]));
    if (borderStyle == null) {
      if (mContext.getCssAlignWithLegacyW3c()) {
        borderStyle = BorderStyle.NONE;
      } else {
        borderStyle = BorderStyle.SOLID;
      }
    }

    mPathEffectForBorderStyle = null;
    switch (borderStyle) {
      case NONE:
      case HIDDEN:
        return;

      case DASHED:
      case DOTTED:
        mPathEffectForBorderStyle = borderStyle.getPathEffect(borderWidthForEffect);
        break;

      case SOLID:
        break;
      case INSET:
        if (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT) {
          borderColor = darkenColor(borderColor);
        }
        break;
      case OUTSET:
        if (borderPosition == Spacing.BOTTOM || borderPosition == Spacing.RIGHT) {
          borderColor = darkenColor(borderColor);
        }
        break;

      case DOUBLE:
        strokeCenterDrawPathMoreLines(
            canvas, borderPosition, borderWidthForEffect / 3.0f, borderColor, borderColor, true);
        return;
      case GROOVE:
        strokeCenterDrawPathMoreLines(canvas, borderPosition, borderWidthForEffect / 2.0f,
            borderColor, darkenColor(borderColor), false);
        return;
      case RIDGE:
        strokeCenterDrawPathMoreLines(canvas, borderPosition, borderWidthForEffect / 2.0f,
            darkenColor(borderColor), borderColor, false);
        return;
    }

    mPaint.setStyle(Paint.Style.STROKE);
    mPaint.setColor(ColorUtil.multiplyColorAlpha(borderColor, mAlpha));
    mPaint.setStrokeWidth(borderWidthForStroke);
    mPaint.setPathEffect(mPathEffectForBorderStyle);
    mPaint.setAntiAlias(true);

    RoundRectPath centerPath = getPathFromCache(RoundRectPath.Pos.CENTER);
    if (centerPath != null) {
      centerPath.drawToCanvas(canvas, mPaint);
    }

    mPaint.setPathEffect(null);
  }

  private int calcBorderMeasureWidth(float width) {
    return (width > 0.1f && width < 1.0f ? 1 : Math.round(width));
  }

  private boolean hasTransparentBorderColor() {
    return (Color.alpha(getBorderColor(Spacing.LEFT)) & Color.alpha(getBorderColor(Spacing.TOP))
               & Color.alpha(getBorderColor(Spacing.RIGHT))
               & Color.alpha(getBorderColor(Spacing.BOTTOM)))
        != 255;
  }

  private boolean isTransparentBorderStyle(BorderStyle style) {
    return style == BorderStyle.DASHED || style == BorderStyle.DOTTED
        || style == BorderStyle.HIDDEN;
  }

  private boolean hasTransparentBorderStyle() {
    return isTransparentBorderStyle(getBorderStyle(Spacing.LEFT))
        || isTransparentBorderStyle(getBorderStyle(Spacing.TOP))
        || isTransparentBorderStyle(getBorderStyle(Spacing.RIGHT))
        || isTransparentBorderStyle(getBorderStyle(Spacing.BOTTOM));
  }

  private boolean hasTransparentBorder() {
    return hasTransparentBorderColor() || hasTransparentBorderStyle();
  }

  private void drawRectangularBorders(Canvas canvas) {
    final RectF borderWidth = getDirectionAwareBorderInsets();

    final int borderLeft = calcBorderMeasureWidth(borderWidth.left);
    final int borderTop = calcBorderMeasureWidth(borderWidth.top);
    final int borderRight = calcBorderMeasureWidth(borderWidth.right);
    final int borderBottom = calcBorderMeasureWidth(borderWidth.bottom);

    final boolean borderWidthToDraw =
        (borderLeft > 0 || borderRight > 0 || borderTop > 0 || borderBottom > 0);

    // maybe draw borders?
    if (borderWidthToDraw) {
      Rect bounds = getBounds();

      int colorLeft = getBorderColor(Spacing.LEFT);
      int colorTop = getBorderColor(Spacing.TOP);
      int colorRight = getBorderColor(Spacing.RIGHT);
      int colorBottom = getBorderColor(Spacing.BOTTOM);

      int left = bounds.left;
      int top = bounds.top;

      mPaint.setAntiAlias(false);
      mPaint.setStyle(Paint.Style.STROKE);

      // Check for fast path to border drawing.
      int fastBorderColor = fastBorderCompatibleColorOrZero(borderLeft, borderTop, borderRight,
          borderBottom, colorLeft, colorTop, colorRight, colorBottom);

      if (fastBorderColor != 0 && toDrawBorderUseSameStyle()) {
        if (Color.alpha(fastBorderColor) != 0) {
          final int right = bounds.right, bottom = bounds.bottom;
          final BorderStyle borderStyle = getBorderStyleWithDefaultSolid(Spacing.LEFT);

          if (borderTop > 0) {
            final float topCenter = top + borderTop * 0.5f;
            // should not cover start of the next section, or if alpha is not 255, the color
            // will be darken
            final float endTop = right - (borderRight > 0 ? borderRight : 0);
            borderStyle.strokeBorderLine(canvas, mPaint, Spacing.TOP, borderWidth.top,
                fastBorderColor, left, topCenter, endTop, topCenter, right - left, borderTop);
          }
          if (borderRight > 0) {
            final float rightCenter = right - borderRight * 0.5f;
            final float endRight = bottom - (borderBottom > 0 ? borderBottom : 0);
            borderStyle.strokeBorderLine(canvas, mPaint, Spacing.RIGHT, borderWidth.right,
                fastBorderColor, rightCenter, top, rightCenter, endRight, bottom - top,
                borderRight);
          }
          if (borderBottom > 0) {
            final float bottomCenter = bottom - borderBottom * 0.5f;
            final float endBottom = left + (borderLeft > 0 ? borderLeft : 0);
            borderStyle.strokeBorderLine(canvas, mPaint, Spacing.BOTTOM, borderWidth.bottom,
                fastBorderColor, right, bottomCenter, endBottom, bottomCenter, right - left,
                borderBottom);
          }
          if (borderLeft > 0) {
            final float leftCenter = left + borderLeft * 0.5f;
            final float endLeft = top + (borderTop > 0 ? borderTop : 0);
            borderStyle.strokeBorderLine(canvas, mPaint, Spacing.LEFT, borderWidth.left,
                fastBorderColor, leftCenter, bottom, leftCenter, endLeft, bottom - top, borderLeft);
          }
        }
      } else {
        // If the path drawn previously is of the same color,
        // there would be a slight white space between borders
        // with anti-alias set to true.
        // Therefore we need to disable anti-alias, and
        // after drawing is done, we will re-enable it.
        int width = bounds.width();
        int height = bounds.height();

        if (borderTop > 0 && Color.alpha(colorTop) != 0) {
          final float x1 = left;
          final float y1 = top;
          final float x2 = left + borderLeft;
          final float y2 = top + borderTop;
          final float x3 = left + width - borderRight;
          final float y3 = top + borderTop;
          final float x4 = left + width;
          final float y4 = top;
          final float y5 = top + borderTop * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, false);
          getBorderStyleWithDefaultSolid(Spacing.TOP)
              .strokeBorderLine(canvas, mPaint, Spacing.TOP, borderWidth.top, colorTop, x1, y5, x4,
                  y5, width, borderTop);
          canvas.restore();
        }

        if (borderRight > 0 && Color.alpha(colorRight) != 0) {
          final float x1 = left + width;
          final float y1 = top;
          final float x2 = left + width;
          final float y2 = top + height;
          final float x3 = left + width - borderRight;
          final float y3 = top + height - borderBottom;
          final float x4 = left + width - borderRight;
          final float y4 = top + borderTop;
          final float x5 = left + width - borderRight * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, false);
          getBorderStyleWithDefaultSolid(Spacing.RIGHT)
              .strokeBorderLine(canvas, mPaint, Spacing.RIGHT, borderWidth.right, colorRight, x5,
                  y1, x5, y2, height, borderRight);
          canvas.restore();
        }

        if (borderBottom > 0 && Color.alpha(colorBottom) != 0) {
          final float x1 = left;
          final float y1 = top + height;
          final float x2 = left + width;
          final float y2 = top + height;
          final float x3 = left + width - borderRight;
          final float y3 = top + height - borderBottom;
          final float x4 = left + borderLeft;
          final float y4 = top + height - borderBottom;
          final float y5 = top + height - borderBottom * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, false);
          getBorderStyleWithDefaultSolid(Spacing.BOTTOM)
              .strokeBorderLine(canvas, mPaint, Spacing.BOTTOM, borderWidth.bottom, colorBottom, x2,
                  y5, x1, y5, width, borderBottom);
          canvas.restore();
        }

        if (borderLeft > 0 && Color.alpha(colorLeft) != 0) {
          final float x1 = left;
          final float y1 = top;
          final float x2 = left + borderLeft;
          final float y2 = top + borderTop;
          final float x3 = left + borderLeft;
          final float y3 = top + height - borderBottom;
          final float x4 = left;
          final float y4 = top + height;
          final float x5 = left + borderLeft * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4, false);
          getBorderStyleWithDefaultSolid(Spacing.LEFT)
              .strokeBorderLine(canvas, mPaint, Spacing.LEFT, borderWidth.left, colorLeft, x5, y4,
                  x5, y1, height, borderLeft);
          canvas.restore();
        }
      }
    }
    mPaint.setAntiAlias(true);
  }

  private void clipQuadrilateral(Canvas canvas, float x1, float y1, float x2, float y2, float x3,
      float y3, float x4, float y4, boolean addBorderClipArea) {
    if (addBorderClipArea) {
      if (mOuterClipPathForBorderRadius != null) {
        canvas.clipPath(mOuterClipPathForBorderRadius.path, Region.Op.INTERSECT);
      }
      if (mInnerClipPathForBorderRadius != null) {
        canvas.clipPath(mInnerClipPathForBorderRadius.path, Region.Op.DIFFERENCE);
      }
    }
    if (mPathForBorder == null) {
      mPathForBorder = new Path();
    }
    mPathForBorder.reset();
    mPathForBorder.moveTo(x1, y1);
    mPathForBorder.lineTo(x2, y2);
    mPathForBorder.lineTo(x3, y3);
    mPathForBorder.lineTo(x4, y4);
    mPathForBorder.lineTo(x1, y1);

    canvas.clipPath(mPathForBorder);
  }

  private int getBorderWidth(int position) {
    if (mBorderWidth == null) {
      return 0;
    }

    final float width = mBorderWidth.get(position);
    return MeasureUtils.isUndefined(width) ? -1 : Math.round(width);
  }

  private static int colorFromAlphaAndRGBComponents(float alpha, float rgb) {
    int rgbComponent = 0x00FFFFFF & (int) rgb;
    int alphaComponent = 0xFF000000 & ((int) alpha) << 24;

    return rgbComponent | alphaComponent;
  }

  private boolean isBorderColorDefined(int position) {
    final float rgb = mBorderRGB != null ? mBorderRGB.get(position) : MeasureUtils.UNDEFINED;
    final float alpha = mBorderAlpha != null ? mBorderAlpha.get(position) : MeasureUtils.UNDEFINED;
    return !MeasureUtils.isUndefined(rgb) && !MeasureUtils.isUndefined(alpha);
  }

  private int getBorderColor(int position) {
    float rgb = mBorderRGB != null ? mBorderRGB.get(position) : DEFAULT_BORDER_RGB;
    float alpha = mBorderAlpha != null ? mBorderAlpha.get(position) : DEFAULT_BORDER_ALPHA;

    return BackgroundDrawable.colorFromAlphaAndRGBComponents(alpha, rgb);
  }

  public Path getInnerClipPathForBorderRadius() {
    if (mBorderCornerRadii != null && updatePath()) {
      if (mInnerClipPathForBorderRadius != null) {
        return mInnerClipPathForBorderRadius.path;
      }
    }

    return null;
  }

  public void setBoxShadowInsetDrawer(UIShadowProxy.InsetDrawer drawer) {
    mBoxShadowInsetDrawer = drawer;
  }

  public UIShadowProxy.InsetDrawer getBoxShadowInsetDrawer() {
    return mBoxShadowInsetDrawer;
  }
}

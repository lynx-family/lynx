// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

@Keep
public interface ILynxTextService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxTextService.class;
  }
  /**
   * Create a TextLayoutAPI Object
   *
   * @param context lynx context for text layout api
   * @return native object pointer of TextLayoutAPI
   */
  long createTextLayoutAPI(Object context);
  /**
   * Destroy a TextLayoutAPI Object
   *
   * @param api native object pointer of TextLayoutAPI
   */
  void destroyTextLayoutAPI(long api);

  /**
   * Get text layout info.
   *
   * @param text text content
   * @param fontSize font size with unit
   * @param fontFamily font family
   * @param maxWidth max measure width with unit
   * @param maxLine max measure line count
   * @return platform text info object
   */
  @Nullable
  LynxTextInfo getTextInfo(String text, String fontSize, @Nullable String fontFamily,
      @Nullable String maxWidth, int maxLine);

  @Keep
  class LynxTextInfo {
    private final double width;
    private final double height;
    @Nullable private final String[] content;

    public LynxTextInfo(double width) {
      this(width, 0, null);
    }

    public LynxTextInfo(double width, @Nullable String[] content) {
      this(width, 0, content);
    }

    public LynxTextInfo(double width, double height) {
      this(width, height, null);
    }

    public LynxTextInfo(double width, double height, @Nullable String[] content) {
      this.width = width;
      this.height = height;
      this.content = content == null ? null : content.clone();
    }

    public double getWidth() {
      return width;
    }

    public double getHeight() {
      return height;
    }

    @Nullable
    public String[] getContent() {
      return content == null ? null : content.clone();
    }
  }

  /**
   * create a page object from native Page
   * @param page native object pointer of Page
   * @return platform Page object
   */
  Page createPage(long page);

  public interface Page {
    /**
     * Draw page on a canvas
     *
     * @param canvas Android canvas
     * @param callback Drawable.Callback
     */
    void drawPageCanvas(Canvas canvas, Drawable.Callback callback);
    /**
     * Get char index from touch position
     *
     * @param touchX touch position x
     * @param touchY touch position y
     * @return index of char on the touch position
     */
    int getSelectionCharIndex(float touchX, float touchY);

    /**
     * Get the TextService inline event target at the touch position.
     *
     * @param touchX touch position x
     * @param touchY touch position y
     * @return packed event target info, every three int values represent [sign, eventMask,
     *         isInlineView].
     */
    default int[] getHitTestEventTargets(float touchX, float touchY) {
      return null;
    }

    /**
     * Get selection rects by char range
     *
     * @param start char index of the start touch position
     * @param end char index of the end touch position
     * @return rect array for each selection line,
     *         every four float value in the returned array represent to a rect of line,
     *         which packed to [left, top, width, height] format.
     */
    float[] getSelectionRects(int start, int end);

    /**
     * Get page text length.
     *
     * @return text length in selection index units
     */
    int getTextLength();

    /**
     * Get selected text content by char range.
     *
     * @param start char index of selected range start
     * @param end char index of selected range end
     * @return selected text
     */
    String getSelectedText(int start, int end);

    /**
     * destroy native page
     */
    void destroy();
  }
}

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;

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

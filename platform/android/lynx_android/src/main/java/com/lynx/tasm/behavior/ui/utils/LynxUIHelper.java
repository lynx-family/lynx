// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import android.content.Context;
import android.graphics.PointF;
import android.graphics.RectF;
import android.view.View;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;

public class LynxUIHelper {
  final static String TAG = "LynxUIHelper";

  /**
   * @param descendant {@link LynxBaseUI} the descendant ui.
   * @param point {@link PointF} the point in descendant ui coordinate.
   * @return The function convertPointFromUIToRootUI converts a point from the coordinate
   *     system of the descendant ui to the coordinate system of the root ui. It returns the
   *     coordinates of the point in the root ui's coordinate system after the conversion.
   */
  public static PointF convertPointFromUIToRootUI(final LynxBaseUI descendant, PointF point) {
    if (descendant == null) {
      LLog.e(TAG, "convertPointFromUIToRootUI failed since descendant ui is null");
      return point;
    }
    LynxContext context = descendant.getLynxContext();
    if (context == null) {
      LLog.e(TAG, "convertPointFromUIToRootUI failed since context is null");
      return point;
    }
    UIBody rootUI = context.getUIBody();
    if (rootUI == null) {
      LLog.e(TAG, "convertPointFromUIToRootUI failed since root ui is null");
      return point;
    }

    if (descendant == rootUI) {
      return point;
    }

    PointF location = new PointF(point.x, point.y);

    View view = null;
    if (descendant.isFlatten()) {
      location.x += descendant.getLeft();
      location.y += descendant.getTop();
      if (descendant.getDrawParent() == null) {
        LLog.e(TAG,
            "mDrawParent of flattenUI is null, which causes the value convertPointFromUIToRootUI returns is not the correct coordinates relative to the root ui!");
        return point;
      }
      view = ((LynxUI) descendant.getDrawParent()).getView();
      location.x -= view.getScrollX();
      location.y -= view.getScrollY();
    } else {
      view = ((LynxUI) descendant).getView();
    }
    return ViewHelper.convertPointFromViewToAnother(view, rootUI.getView(), location);
  }

  /**
   * @param ui {@link LynxBaseUI} the descendant ui.
   * @param point {@link PointF} the point in descendant ui coordinate.
   * @return The function convertPointFromUIToScreen converts a point from the coordinate
   *     system of the descendant ui to the coordinate system of the screen. It returns the
   *     coordinates of the point in the screen's coordinate system after the conversion.
   */
  public static PointF convertPointFromUIToScreen(final LynxBaseUI ui, PointF point) {
    if (ui == null) {
      LLog.e(TAG, "convertPointFromUIToScreen failed since ui is null");
      return point;
    }

    PointF location = new PointF(point.x, point.y);

    View view = null;
    if (ui.isFlatten()) {
      location.x += ui.getLeft();
      location.y += ui.getTop();
      if (ui.getDrawParent() == null) {
        LLog.e(TAG,
            "mDrawParent of flattenUI is null, which causes the value convertPointFromUIToScreen returns is not the correct coordinates relative to the screen!");
        return point;
      }
      view = ((LynxUI) ui.getDrawParent()).getView();
      location.x -= view.getScrollX();
      location.y -= view.getScrollY();
    } else {
      view = ((LynxUI) ui).getView();
    }
    return ViewHelper.convertPointFromDescendantToAncestor(view, view.getRootView(), location);
  }

  /**
   * @param descendant {@link LynxBaseUI} the descendant ui.
   * @param rect {@link RectF} the rect in descendant ui coordinate.
   * @return The function convertRectFromUIToRootUI converts a rect from the coordinate
   *     system of the descendant ui to the coordinate system of the root ui. It returns the
   *     coordinates of the rect in the root ui's coordinate system after the conversion.
   */
  public static RectF convertRectFromUIToRootUI(final LynxBaseUI descendant, RectF rect) {
    LynxContext context = descendant.getLynxContext();
    if (context == null) {
      LLog.e(TAG, "convertRectFromUIToRootUI failed since context is null");
      return rect;
    }
    UIBody rootUI = context.getUIBody();
    if (rootUI == null) {
      LLog.e(TAG, "convertRectFromUIToRootUI failed since rootUI is null");
      return rect;
    }

    return convertRectFromUIToAnotherUI(descendant, rootUI, rect);
  }

  /**
   * @param ui {@link LynxBaseUI} the ui in which the initial rect are located.
   * @param another {@link  LynxBaseUI} the ui in which the rect are located after the conversion.
   * @param rect {@link RectF} the rect in the ui coordinate.
   * @return The function convertRectFromUIToAnotherUI converts a rect from the coordinate
   *     system of the ui to the coordinate system of the another ui. It returns the
   *     coordinates of the rect in the another ui's coordinate system after the conversion.
   */
  public static RectF convertRectFromUIToAnotherUI(
      final LynxBaseUI ui, final LynxBaseUI another, RectF rect) {
    if (ui == null) {
      LLog.e(TAG, "convertRectFromUIToRootUI failed since descendant is null");
      return rect;
    }

    if (another == null) {
      LLog.e(TAG, "convertRectFromUIToRootUI failed since another ui is null");
      return rect;
    }

    if (ui == another) {
      return rect;
    }

    RectF targetRect = new RectF(rect.left, rect.top, rect.right, rect.bottom);

    View view = null;
    if (ui.isFlatten()) {
      targetRect.left += ui.getLeft();
      targetRect.top += ui.getTop();

      if (ui.getDrawParent() == null) {
        LLog.e(TAG,
            "mDrawParent of flattenUI is null, which causes the value convertRectFromUIToAnotherUI returns is not the correct coordinates relative to the another ui!");
        return rect;
      }
      view = ((LynxUI) ui.getDrawParent()).getView();

      targetRect.left -= view.getScrollX();
      targetRect.top -= view.getScrollY();
      targetRect.right = targetRect.left + rect.width();
      targetRect.bottom = targetRect.top + rect.height();
    } else {
      view = ((LynxUI) ui).getView();
    }

    View anotherView = null;
    float offsetX = 0, offsetY = 0;
    if (another.isFlatten()) {
      offsetX += another.getLeft();
      offsetY += another.getTop();

      if (another.getDrawParent() == null) {
        LLog.e(TAG,
            "mDrawParent of flattenUI is null, which causes the value convertRectFromUIToAnotherUI returns is not the correct coordinates relative to the another ui!");
        return rect;
      }
      anotherView = ((LynxUI) another.getDrawParent()).getView();

      offsetX -= anotherView.getScrollX();
      offsetY -= anotherView.getScrollY();
    } else {
      anotherView = ((LynxUI) another).getView();
    }

    RectF res = ViewHelper.convertRectFromViewToAnother(view, anotherView, targetRect);
    res.offset(-offsetX, -offsetY);
    return res;
  }

  /**
   * @param ui {@link LynxBaseUI} the descendant ui.
   * @param rect {@link RectF} the rect in descendant ui coordinate.
   * @return The function convertRectFromUIToScreen converts a rect from the coordinate
   *     system of the descendant ui to the coordinate system of the screen. It returns the
   *     coordinates of the rect in the screen's coordinate system after the conversion.
   */
  public static RectF convertRectFromUIToScreen(final LynxBaseUI ui, RectF rect) {
    if (ui == null) {
      LLog.e(TAG, "convertRectFromUIToScreen failed since ancestor or descendant is null");
      return rect;
    }

    RectF targetRect = new RectF(rect.left, rect.top, rect.right, rect.bottom);

    View view = null;
    if (ui.isFlatten()) {
      targetRect.left += ui.getLeft();
      targetRect.top += ui.getTop();

      if (ui.getDrawParent() == null) {
        LLog.e(TAG,
            "mDrawParent of flattenUI is null, which causes the value convertRectFromUIToScreen returns is not the correct coordinates relative to the screen!");
        return rect;
      }
      view = ((LynxUI) ui.getDrawParent()).getView();

      targetRect.left -= view.getScrollX();
      targetRect.top -= view.getScrollY();
      targetRect.right = targetRect.left + rect.width();
      targetRect.bottom = targetRect.top + rect.height();
    } else {
      view = ((LynxUI) ui).getView();
    }
    return ViewHelper.convertRectFromDescendantToAncestor(view, view.getRootView(), targetRect);
  }

  /**
   * Converts a pixel value to density-independent pixels (dip).
   *
   * @param pxValue The pixel value to convert.
   * @return  The converted value in density-independent pixels.
   */
  public static int px2dip(Context context, float pxValue) {
    if (context == null || context.getResources() == null
        || context.getResources().getDisplayMetrics() == null) {
      return (int) pxValue;
    }
    float scale = context.getResources().getDisplayMetrics().density;
    return (int) (pxValue / scale + 0.5);
  }

  /**
   * @param ui {@link LynxBaseUI} The UI that initiates the query.
   * @param relativeID {@link String} The id selector provided by the front end.
   * @return The ui corresponding to id selector. First search upwards, if not found,
   *         then search globally.
   */
  public static LynxBaseUI getRelativeUI(final LynxBaseUI ui, String relativeID) {
    if (ui == null || relativeID == null) {
      LLog.e(TAG, "getRelativeUI failed since ui or relativeID is null");
      return null;
    }

    LynxBaseUI uiParent = (LynxBaseUI) ui.getParent();
    while (uiParent instanceof LynxBaseUI) {
      if (relativeID.equals(uiParent.getIdSelector())) {
        return uiParent;
      }
      uiParent = (LynxBaseUI) uiParent.getParent();
    }

    if (ui.getLynxContext() == null) {
      LLog.e(TAG, "getRelativeUI failed since context is null");
      return null;
    }

    if (ui.getLynxContext().getLynxUIOwner() == null) {
      LLog.e(TAG, "getRelativeUI failed since uiowner is null");
      return null;
    }

    return ui.getLynxContext().getLynxUIOwner().findLynxUIByIdSelector(relativeID);
  }

  /**
   * @param ui {@link LynxBaseUI} The UI that initiates the query.
   * @param params {@link ReadableMap} The query parameters, including androidEnableTransformProps
   *     and relativeTo.
   * @return The location information of ui. It may be after transform, or it may be relative to a
   *     certain UI.
   */
  public static RectF getRelativePositionInfo(final LynxBaseUI ui, ReadableMap params) {
    if (params != null && params.getBoolean(PropsConstants.ANDROID_ENABLE_TRANSFORM_PROPS, false)) {
      String relativeID =
          (params == null || !params.hasKey("relativeTo") || params.getString("relativeTo") == null)
          ? null
          : params.getString("relativeTo");
      LynxBaseUI relativeUI = getRelativeUI(ui, relativeID);
      if ("screen".equals(relativeID)) {
        return LynxUIHelper.convertRectFromUIToScreen(
            ui, new RectF(0, 0, ui.getWidth(), ui.getHeight()));
      } else if (relativeUI != null) {
        return LynxUIHelper.convertRectFromUIToAnotherUI(
            ui, relativeUI, new RectF(0, 0, ui.getWidth(), ui.getHeight()));
      } else {
        return LynxUIHelper.convertRectFromUIToRootUI(
            ui, new RectF(0, 0, ui.getWidth(), ui.getHeight()));
      }
    }

    return new RectF(ui.getBoundingClientRect());
  }
}

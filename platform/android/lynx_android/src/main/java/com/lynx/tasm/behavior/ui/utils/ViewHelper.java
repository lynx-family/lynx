// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import com.lynx.tasm.base.LLog;
import java.util.ArrayList;

public class ViewHelper {
  final static String TAG = "ViewHelper";

  /**
   * Set the background to a given Drawable, or remove the background
   */
  public static void setBackground(View view, Drawable drawable) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
      view.setBackground(drawable);
    } else {
      view.setBackgroundDrawable(drawable);
    }
  }

  /**
   * @param parent {@link View} the parent view.
   * @param child {@link View} the child view.
   * @return The function viewIsParentOfAnotherView is used to determine if a view parent is an
   *     ancestor view child.
   * 1. If either the parent or child is null, return false.
   * 2. If the parent is not a ViewGroup, return false.
   * 3. If the above two conditions are not satisfied, traverse through the child's parent views and
   * check if it is equal to the parent view.
   */
  public static boolean viewIsParentOfAnotherView(View parent, View child) {
    if (parent == null || child == null) {
      return false;
    }
    if (!(parent instanceof ViewGroup)) {
      return false;
    }
    while (child.getParent() != null) {
      if (child.getParent() == parent) {
        return true;
      }
      // If child.getParent() instanceof View, continue the loop. Otherwise, return false.
      if (child.getParent() instanceof View) {
        child = (View) child.getParent();
      } else {
        return false;
      }
    }
    return false;
  }

  /**
   * @param ancestor {@link View} the ancestor view.
   * @param descendant {@link View} the descendant view.
   * @param point {@link PointF} the point in ancestor view's coordinate.
   * @return The function convertPointFromAncestorToDescendant converts a point from the coordinate
   *     system of the ancestor view to the coordinate system of the descendant view. It returns the
   *     coordinates of the point in the descendant view's coordinate system after the conversion.
   */
  public static PointF convertPointFromAncestorToDescendant(
      final View ancestor, final View descendant, PointF point) {
    if (ancestor == null || descendant == null) {
      LLog.e(
          TAG, "convertPointFromAncestorToDescendant failed since ancestor or descendant is null");
      return point;
    }
    if (ancestor == descendant) {
      return point;
    }

    float[] location = {point.x, point.y};

    ArrayList<View> viewChain = new ArrayList<View>();

    View currentView = descendant;
    while (currentView != null && currentView != ancestor) {
      viewChain.add(currentView);
      currentView = (View) currentView.getParent();
    }

    View parentView = ancestor;
    Matrix inverseMatrix = new Matrix();
    for (int i = viewChain.size() - 1; i >= 0; --i) {
      currentView = viewChain.get(i);

      location[0] += parentView.getScrollX();
      location[1] += parentView.getScrollY();

      location[0] -= currentView.getLeft();
      location[1] -= currentView.getTop();

      if (!currentView.getMatrix().isIdentity()) {
        inverseMatrix.reset();

        if (currentView.getMatrix().invert(inverseMatrix)) {
          float[] tempPoint = {location[0], location[1]};
          inverseMatrix.mapPoints(tempPoint);
          location[0] = tempPoint[0];
          location[1] = tempPoint[1];
        } else {
          location[0] = Float.MAX_VALUE;
          location[1] = Float.MAX_VALUE;
        }
      }

      parentView = currentView;
    }

    return new PointF(location[0], location[1]);
  }

  /**
   * @param descendant {@link View} the descendant view.
   * @param ancestor {@link View} the ancestor view.
   * @param point {@link PointF} the point in descendant view's coordinate.
   * @return The function convertPointFromDescendantToAncestor converts a point from the coordinate
   *     system of the descendant view to the coordinate system of the ancestor view. It returns the
   *     coordinates of the point in the ancestor view's coordinate system after the conversion.
   */
  public static PointF convertPointFromDescendantToAncestor(
      final View descendant, final View ancestor, PointF point) {
    if (ancestor == null || descendant == null) {
      LLog.e(TAG, "convertPointFromAncestorToDescendant failed since view or another is null");
      return point;
    }
    if (ancestor == descendant) {
      return point;
    }

    float[] location = {point.x, point.y};
    if (!descendant.getMatrix().isIdentity()) {
      descendant.getMatrix().mapPoints(location);
    }

    View currentView = descendant;

    while (currentView != ancestor) {
      final View parentView = (View) currentView.getParent();
      if (parentView == null) {
        LLog.e(TAG, "convertPointFromDescendantToAncestor failed, parent is null.");
        break;
      }

      location[0] += currentView.getLeft();
      location[1] += currentView.getTop();

      location[0] -= parentView.getScrollX();
      location[1] -= parentView.getScrollY();

      if (!parentView.getMatrix().isIdentity()) {
        parentView.getMatrix().mapPoints(location);
      }

      currentView = parentView;
    }
    return new PointF(location[0], location[1]);
  }

  /**
   * @param view {@link View} the view in which the initial coordinates are located.
   * @param another {@link View} the view in which the coordinates are located after the conversion.
   * @param point {@link PointF} the point in view's coordinate.
   * @return The function convertPointFromViewToAnother converts a point from the coordinate system
   *     of the view to the coordinate system of another view. It returns the coordinates of the
   *     point in the ancestor another view's coordinate system after the conversion.
   */
  public static PointF convertPointFromViewToAnother(
      final View view, final View another, PointF point) {
    if (view == null || another == null) {
      LLog.e(TAG, "convertPointFromAncestorToDescendant failed since view or another is null");
      return point;
    }
    if (view == another) {
      return point;
    }

    if (viewIsParentOfAnotherView(view, another)) {
      return convertPointFromAncestorToDescendant(view, another, point);
    } else if (viewIsParentOfAnotherView(another, view)) {
      return convertPointFromDescendantToAncestor(view, another, point);
    } else {
      View rootView = view.getRootView();
      PointF rootViewPoint = convertPointFromDescendantToAncestor(view, rootView, point);

      View anotherRootView = another.getRootView();
      return convertPointFromAncestorToDescendant(anotherRootView, another, rootViewPoint);
    }
  }

  /**
   * @param ancestor {@link View} the ancestor view.
   * @param descendant {@link View} the descendant view.
   * @param rect {@link RectF} the rect in ancestor view's coordinate.
   * @return The function convertRectFromAncestorToDescendant converts a rect from the coordinate
   *     system of the ancestor view to the coordinate system of the descendant view. It returns the
   *     coordinates of the rect in the descendant view's coordinate system after the conversion.
   */
  public static RectF convertRectFromAncestorToDescendant(
      final View ancestor, final View descendant, RectF rect) {
    if (ancestor == null || descendant == null) {
      LLog.e(
          TAG, "convertPointFromAncestorToDescendant failed since ancestor or descendant is null");
      return rect;
    }
    if (ancestor == descendant) {
      return rect;
    }

    PointF locationLeftTop = new PointF(rect.left, rect.top);
    PointF locationRightTop = new PointF(rect.right, rect.top);
    PointF locationLeftBottom = new PointF(rect.left, rect.bottom);
    PointF locationRightBottom = new PointF(rect.right, rect.bottom);

    PointF ancestorLocationLeftTop =
        convertPointFromAncestorToDescendant(ancestor, descendant, locationLeftTop);
    PointF ancestorLocationRightTop =
        convertPointFromAncestorToDescendant(ancestor, descendant, locationRightTop);
    PointF ancestorLocationLeftBottom =
        convertPointFromAncestorToDescendant(ancestor, descendant, locationLeftBottom);
    PointF ancestorLocationRightBottom =
        convertPointFromAncestorToDescendant(ancestor, descendant, locationRightBottom);

    RectF convertedRect = new RectF();
    convertedRect.left = Math.min(Math.min(ancestorLocationLeftTop.x, ancestorLocationRightTop.x),
        Math.min(ancestorLocationLeftBottom.x, ancestorLocationRightBottom.x));
    convertedRect.top = Math.min(Math.min(ancestorLocationLeftTop.y, ancestorLocationRightTop.y),
        Math.min(ancestorLocationLeftBottom.y, ancestorLocationRightBottom.y));
    convertedRect.right = Math.max(Math.max(ancestorLocationLeftTop.x, ancestorLocationRightTop.x),
        Math.max(ancestorLocationLeftBottom.x, ancestorLocationRightBottom.x));
    convertedRect.bottom = Math.max(Math.max(ancestorLocationLeftTop.y, ancestorLocationRightTop.y),
        Math.max(ancestorLocationLeftBottom.y, ancestorLocationRightBottom.y));

    return convertedRect;
  }

  /**
   * @param descendant {@link View} the descendant view.
   * @param ancestor {@link View} the ancestor view.
   * @param rect {@link RectF} the rect in descendant view's coordinate.
   * @return The function convertRectFromDescendantToAncestor converts a rect from the coordinate
   *     system of the descendant view to the coordinate system of the ancestor view. It returns the
   *     coordinates of the rect in the ancestor view's coordinate system after the conversion.
   */
  public static RectF convertRectFromDescendantToAncestor(
      final View descendant, final View ancestor, RectF rect) {
    if (ancestor == null || descendant == null) {
      LLog.e(
          TAG, "convertPointFromAncestorToDescendant failed since ancestor or descendant is null");
      return rect;
    }
    if (ancestor == descendant) {
      return rect;
    }

    PointF locationLeftTop = new PointF(rect.left, rect.top);
    PointF locationRightTop = new PointF(rect.right, rect.top);
    PointF locationLeftBottom = new PointF(rect.left, rect.bottom);
    PointF locationRightBottom = new PointF(rect.right, rect.bottom);

    PointF ancestorLocationLeftTop =
        convertPointFromDescendantToAncestor(descendant, ancestor, locationLeftTop);
    PointF ancestorLocationRightTop =
        convertPointFromDescendantToAncestor(descendant, ancestor, locationRightTop);
    PointF ancestorLocationLeftBottom =
        convertPointFromDescendantToAncestor(descendant, ancestor, locationLeftBottom);
    PointF ancestorLocationRightBottom =
        convertPointFromDescendantToAncestor(descendant, ancestor, locationRightBottom);

    RectF convertedRect = new RectF();
    convertedRect.left = Math.min(Math.min(ancestorLocationLeftTop.x, ancestorLocationRightTop.x),
        Math.min(ancestorLocationLeftBottom.x, ancestorLocationRightBottom.x));
    convertedRect.top = Math.min(Math.min(ancestorLocationLeftTop.y, ancestorLocationRightTop.y),
        Math.min(ancestorLocationLeftBottom.y, ancestorLocationRightBottom.y));
    convertedRect.right = Math.max(Math.max(ancestorLocationLeftTop.x, ancestorLocationRightTop.x),
        Math.max(ancestorLocationLeftBottom.x, ancestorLocationRightBottom.x));
    convertedRect.bottom = Math.max(Math.max(ancestorLocationLeftTop.y, ancestorLocationRightTop.y),
        Math.max(ancestorLocationLeftBottom.y, ancestorLocationRightBottom.y));

    return convertedRect;
  }

  /**
   * @param view {@link View} the view in which the initial rect are located.
   * @param another {@link View} the view in which the rect are located after the conversion.
   * @param rect {@link RectF} the rect in view's coordinate.
   * @return The function convertRectFromViewToAnother converts a rect from the coordinate system of
   *     the view to the coordinate system of another view. It returns the coordinates of the rect
   *     in the ancestor another view's coordinate system after the conversion.
   */
  public static RectF convertRectFromViewToAnother(
      final View view, final View another, RectF rect) {
    if (view == null || another == null) {
      LLog.e(TAG, "convertRectFromViewToAnother failed since view or another is null");
      return rect;
    }
    if (view == another) {
      return rect;
    }

    if (viewIsParentOfAnotherView(view, another)) {
      return convertRectFromAncestorToDescendant(view, another, rect);
    } else if (viewIsParentOfAnotherView(another, view)) {
      return convertRectFromDescendantToAncestor(view, another, rect);
    } else {
      View rootView = view.getRootView();
      RectF rootViewRect = convertRectFromDescendantToAncestor(view, rootView, rect);

      View anotherRootView = another.getRootView();
      return convertRectFromAncestorToDescendant(anotherRootView, another, rootViewRect);
    }
  }
}

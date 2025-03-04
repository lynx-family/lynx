// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Point;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Gravity;
import android.view.Surface;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;

public class KeyboardMonitor extends Dialog {
  private static final String TAG = KeyboardMonitor.class.getSimpleName();

  public KeyboardMonitor(@NonNull Context context) {
    super(context);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    getWindow().setGravity(Gravity.LEFT);
    getWindow().setBackgroundDrawable(new ColorDrawable(0));
    getWindow().setLayout(2, WindowManager.LayoutParams.MATCH_PARENT);
    getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
    /* Modify some flags that may cause the window to become full screen and
     * cause ADJUST_RESIZE to not work.
     */
    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN);
    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
    getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
    /* disabled shadow
     * -----
     * RenderNode::hasShadow reminder :
     * Checks if the RenderNode has a shadow. That is, if the combination of getElevation()
     * and getTranslationZ() is greater than zero, there is an Outline set with
     * a valid shadow caster path, and the provided outline has a non-zero getAlpha().
     * -----
     * So we should set elevation and translationZ as 0 to avoid hasShadow=true in some rom
     */
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      getDecorView().setOutlineProvider(null);
      getDecorView().setElevation(0);
      getDecorView().setTranslationZ(0);
    }
  }

  /**
   * Return the bottom coordinate of the KeyboardMonitor in the current display orientation.
   *
   * <p>Note that we assume the KeyboardMonitor always fills the screen along with the LynxView!
   * We cannot guarantee that the correct point can be returned if this condition is not met!</p>
   *
   * <p>The KeyboardMonitor measures the keyboard height through changes in its own height, so we
   * need to obtain the accurate initial height. Since Android allows the keyboard to be
   * persistently displayed (by setting {@link
   * android.view.WindowManager.LayoutParams#SOFT_INPUT_STATE_ALWAYS_VISIBLE}), we may get a height
   * that has already been compressed by the keyboard if we directly get the DecorView.height
   * through various methods. Therefore, we need to use the change in the bottom coordinate to
   * calculate the keyboard height.</p>
   *
   * @return the original bottom position of the KeyboardMonitor, in pixels.
   */
  public int getDefaultMonitorBottom() {
    DisplayMetrics availableMetrics = new DisplayMetrics();
    Display defaultDisplay = getWindow().getWindowManager().getDefaultDisplay();
    Point smallestSize = new Point();
    Point largestSize = new Point();
    defaultDisplay.getCurrentSizeRange(smallestSize, largestSize);
    defaultDisplay.getMetrics(availableMetrics);
    int screenHeight = availableMetrics.heightPixels;
    int rotation = defaultDisplay.getRotation();
    if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
      if (largestSize.y < screenHeight) {
        // on some ROMs, if virtual buttons are enabled, the size returned by DisplayMetrics.height
        // may be larger than the app's maximum height.
        screenHeight = largestSize.y;
      }
      int statusBarHeight = 0;
      int resourceId =
          getContext().getResources().getIdentifier("status_bar_height", "dimen", "android");
      if (resourceId > 0) {
        statusBarHeight = getContext().getResources().getDimensionPixelSize(resourceId);
      }
      return screenHeight + statusBarHeight;
    } else {
      return screenHeight;
    }
  }

  public View getDecorView() {
    return getWindow().getDecorView();
  }

  private static Activity getActivity(Context context) {
    if (context instanceof Activity) {
      return (Activity) context;
    } else if (context instanceof ContextWrapper) {
      return getActivity(((ContextWrapper) context).getBaseContext());
    } else {
      throw new IllegalArgumentException();
    }
  }

  public void start() {
    if (!isShowing()) {
      try {
        if (!(getActivity(getContext()).isFinishing())) {
          show();
        }
      } catch (WindowManager.BadTokenException e) {
        LLog.w(TAG, e.toString());
      } catch (RuntimeException e) {
        LLog.w(TAG, e.toString());
      }
    }
  }

  public void stop() {
    if (isShowing()) {
      try {
        dismiss();
      } catch (WindowManager.BadTokenException e) {
        LLog.w(TAG, e.toString());
      } catch (RuntimeException e) {
        LLog.w(TAG, e.toString());
      }
    }
  }

  public void addOnGlobalLayoutListener(ViewTreeObserver.OnGlobalLayoutListener listener) {
    getWindow().getDecorView().getViewTreeObserver().addOnGlobalLayoutListener(listener);
  }

  public void removeOnGlobalLayoutListener(ViewTreeObserver.OnGlobalLayoutListener listener) {
    getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(listener);
  }
}

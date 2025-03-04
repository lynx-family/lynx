/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.tasm.utils;

import android.content.Context;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.Assertions;
import com.lynx.tasm.base.LLog;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Holds an instance of the current DisplayMetrics so we don't have to thread it through all the
 * classes that need it.
 * Note: windowDisplayMetrics are deprecated in favor of ScreenDisplayMetrics: window metrics
 * are supposed to return the drawable area but there's no guarantee that they correspond to the
 * actual size of the {@link ReactRootView}. Moreover, they are not consistent with what iOS
 * returns. Screen metrics returns the metrics of the entire screen, is consistent with iOS and
 * should be used instead.
 *
 * However, though ideally display metrics should be the real display metrics,
 * sWindowDisplayMetrics is not always in consistent with real screen display metrics.
 * Lynx view operates inside a sandboxed world. And lynx view operates with display metrics
 * imposed by the sandbox. So that the global ScreenDisplayMetrics here stands for metrics of the
 * entire screen inside the sandboxed world where lynx operates.
 * Ideally each lynx view should have its own sandboxed world, but now all lynx views are sharing
 * one global screen density, and one global screen width in x-elements.
 * So do NOT use the global screen metrics here if the lynx env is available.
 * Should NEVER use metrics from system inside lynx components.
 * FIXME: 1st Lynx should keep only one set of global display metrics.
 * FIXME: 2nd Lynx should not keep any global display metrics.
 */
public class DisplayMetricsHolder {
  private static @Nullable DisplayMetrics sWindowDisplayMetrics;
  private static @Nullable DisplayMetrics sScreenDisplayMetrics;
  /**
   * @deprecated screen metrics contains orientation information.
   */
  @Deprecated private static int sOrientation = -1;
  /**
   * @deprecated screen metrics contains density information.
   */
  @Deprecated private static float sScaleDensity = -1;

  private static boolean hasNativeUpdateDeviceInfo = false;
  private static boolean isCacheInvalid = false;

  public final static int UNDEFINE_SCREEN_SIZE_VALUE = -1;
  public final static float DEFAULT_SCREEN_SCALE = 1;

  /**
   * @deprecated Use {@link #setScreenDisplayMetrics(DisplayMetrics)} instead. See comment above as
   *    to why this is not correct to use.
   */
  private static void setWindowDisplayMetrics(DisplayMetrics displayMetrics) {
    synchronized (DisplayMetricsHolder.class) {
      if (sWindowDisplayMetrics == null) {
        sWindowDisplayMetrics = new DisplayMetrics();
      }
      sWindowDisplayMetrics.setTo(displayMetrics);
    }
  }

  /**
   * @deprecated Use {@link com.lynx.tasm.LynxView#updateScreenMetrics(int, int)}instead. See
   *     comment above as
   *      to why this is not correct to use.
   */
  public static boolean updateOrInitDisplayMetrics(Context context) {
    return updateOrInitDisplayMetrics(context, null);
  }

  /**
   * FIXME: Temporary method to update the deprecated global screen metrics before removing it.
   *        Remove this function when global variable get removed.
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public static void updateDisplayMetrics(int width, int height) {
    synchronized (DisplayMetricsHolder.class) {
      // When other views call this method, the cache becomes invalid, so we must use the system's
      // interface to get real screen displayMetrics.
      // TODO(chennengshi): will refactor in the futrue.
      isCacheInvalid = true;
      if (sWindowDisplayMetrics != null) {
        sWindowDisplayMetrics.widthPixels = width;
        sWindowDisplayMetrics.heightPixels = height;
      }
      if (sScreenDisplayMetrics != null) {
        sScreenDisplayMetrics.widthPixels = width;
        sScreenDisplayMetrics.heightPixels = height;
      }
    }
  }

  /**
   * Update or init display metrics.
   *
   * @param context context of current activity
   * @param densityOverride density override
   * @return if screen metrics got updated
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public static boolean updateOrInitDisplayMetrics(Context context, Float densityOverride) {
    if (context == null) {
      LLog.w(LynxConstants.TAG,
          "updateOrInitDisplayMetrics context parameter is null, fallback to updateOrInitDisplayMetrics by ApplicationContext");
      context = LynxEnv.inst().getAppContext();
    }
    updateWindowDisplayMetrics(context, densityOverride);

    boolean needUpdateScreenMetrics = shouldUpdateScreenMetrics(context, densityOverride);
    // When other views call this method, the cache becomes invalid, so we must use the system's
    // interface to get real screen displayMetrics.
    if (needUpdateScreenMetrics || isCacheInvalid) {
      updateScreenDisplayMetrics(context, densityOverride);
      // cache is valid again.
      isCacheInvalid = false;
    }

    updateCurrentProps(context);

    return needUpdateScreenMetrics;
  }

  private static void updateWindowDisplayMetrics(Context context, Float densityOverride) {
    DisplayMetrics windowDM = context.getResources().getDisplayMetrics();
    if (densityOverride != null) {
      windowDM.density = densityOverride;
    }
    DisplayMetricsHolder.setWindowDisplayMetrics(windowDM);
  }

  private static void updateScreenDisplayMetrics(Context context, Float densityOverride) {
    DisplayMetrics displayMetrics = getRealScreenDisplayMetrics(context);
    if (densityOverride != null) {
      displayMetrics.density = densityOverride;
    }
    DisplayMetricsHolder.setScreenDisplayMetrics(displayMetrics);
  }

  private static boolean isScaleDensityChange(DisplayMetrics windowDisplayMetrics) {
    return sScaleDensity != windowDisplayMetrics.scaledDensity;
  }

  private static boolean isDensityChanged(Float densityOverride) {
    return sScreenDisplayMetrics != null && densityOverride != null
        && sScreenDisplayMetrics.density != densityOverride;
  }

  private static boolean isOrientationChanged(Context context) {
    return sOrientation != context.getResources().getConfiguration().orientation;
  }

  private static void updateCurrentProps(Context context) {
    sScaleDensity = context.getResources().getDisplayMetrics().scaledDensity;
    sOrientation = context.getResources().getConfiguration().orientation;
  }

  private static boolean shouldUpdateScreenMetrics(Context context, Float densityOverride) {
    DisplayMetrics windowDM = context.getResources().getDisplayMetrics();
    return DisplayMetricsHolder.getScreenDisplayMetrics() == null || isOrientationChanged(context)
        || isScaleDensityChange(windowDM) || !hasNativeUpdateDeviceInfo
        || isDensityChanged(densityOverride);
  }

  /**
   * Util function to get current real screen metrics
   * @param context current activity context
   * @return metrics of real screen
   */
  public static DisplayMetrics getRealScreenDisplayMetrics(Context context) {
    DisplayMetrics screenDisplayMetrics = new DisplayMetrics();
    DisplayMetrics windowDM = getWindowDisplayMetrics();
    if (windowDM != null) {
      screenDisplayMetrics.setTo(getWindowDisplayMetrics());
    }
    WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
    Assertions.assertNotNull(wm, "WindowManager is null!");
    Display display = wm.getDefaultDisplay();

    // Get the real display metrics if we are using API level 17 or higher.
    // The real metrics include system decor elements (e.g. soft menu bar).
    //
    // See:
    // http://developer.android.com/reference/android/view/Display.html#getRealMetrics(android.util.DisplayMetrics)
    if (Build.VERSION.SDK_INT >= 17) {
      display.getRealMetrics(screenDisplayMetrics);
    } else {
      // For 14 <= API level <= 16, we need to invoke getRawHeight and getRawWidth to get the real
      // dimensions. Since react-native only supports API level 16+ we don't have to worry about
      // other cases.
      //
      // Reflection exceptions are rethrown at runtime.
      //
      // See:
      // http://stackoverflow.com/questions/14341041/how-to-get-real-screen-height-and-width/23861333#23861333
      try {
        Method mGetRawH = Display.class.getMethod("getRawHeight");
        Method mGetRawW = Display.class.getMethod("getRawWidth");
        screenDisplayMetrics.widthPixels = (Integer) mGetRawW.invoke(display);
        screenDisplayMetrics.heightPixels = (Integer) mGetRawH.invoke(display);
      } catch (InvocationTargetException | IllegalAccessException | NoSuchMethodException e) {
        throw new RuntimeException("Error getting real dimensions for API level < 17", e);
      }
    }

    return screenDisplayMetrics;
  }

  /**
   * @deprecated Use {@link com.lynx.tasm.behavior.LynxContext#getScreenMetrics()} instead.
   * See comment above as to why this is not correct to use.
   */
  @Deprecated
  public static DisplayMetrics getWindowDisplayMetrics() {
    synchronized (DisplayMetricsHolder.class) {
      if (sWindowDisplayMetrics == null) {
        return null;
      }
      DisplayMetrics dm = new DisplayMetrics();
      dm.setTo(sWindowDisplayMetrics);
      return dm;
    }
  }

  /**
   * @deprecated Use {@link com.lynx.tasm.LynxView#updateScreenMetrics(int, int)}instead. See
   *     comment above as
   *      to why this is not correct to use.
   */
  private static void setScreenDisplayMetrics(DisplayMetrics dm) {
    boolean isNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
    synchronized (DisplayMetricsHolder.class) {
      sScreenDisplayMetrics = dm;
      if (isNativeLibraryLoaded) {
        hasNativeUpdateDeviceInfo = true;
        nativeUpdateDevice(dm.widthPixels, dm.heightPixels, dm.density);
      }
    }
  }

  /**
   * @deprecated Use {@link com.lynx.tasm.behavior.LynxContext#getScreenMetrics()} instead.
   * See comment above as to why this is not correct to use.
   */
  @Deprecated
  public static DisplayMetrics getScreenDisplayMetrics() {
    synchronized (DisplayMetricsHolder.class) {
      if (sScreenDisplayMetrics == null) {
        return null;
      }
      DisplayMetrics dm = new DisplayMetrics();
      dm.setTo(sScreenDisplayMetrics);
      return dm;
    }
  }

  static native void nativeUpdateDevice(int width, int height, float density);
}

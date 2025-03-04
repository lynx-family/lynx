// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static android.view.Display.DEFAULT_DISPLAY;

import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.text.TextPaint;
import android.text.TextUtils;
import android.view.Display;
import android.view.WindowManager;
import android.widget.TextView;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import java.util.Locale;

/**
 * DeviceUtils is a util class to get device information.
 * We need to do some adaptions for different devices.
 * For example, the font size on different devices may be different,
 * so we need to adapt the font size.
 */
public class DeviceUtils {
  private static boolean sIsMiuiInited = false;
  private static boolean sIsMiui = false;

  private static final String TAG = "DeviceUtils";
  private static final int DEVICE_ARCH_TYPE_UNDEFINED = -1;
  private static final int DEVICE_ARCH_TYPE_32IT = 0;
  private static final int DEVICE_ARCH_TYPE_64BIT = 1;
  private static final String UNKNOWN_CPU_ABI = "unknown";
  public static final int DEFAULT_DEVICE_REFRESH_RATE = 60;

  private static int sIs64Bit = DEVICE_ARCH_TYPE_UNDEFINED;
  private static String sCpuABI = null;

  public synchronized static boolean isMiui() {
    if (!sIsMiuiInited) {
      try {
        Class<?> clz = Class.forName("miui.os.Build");
        if (clz != null) {
          sIsMiui = true;
        }
      } catch (Exception e) {
        // ignore
      }
      sIsMiuiInited = true;
    }
    return sIsMiui;
  }

  private static Typeface sDefaultTypeface;
  private static boolean sIsTypefaceInit;

  public synchronized static Typeface getDefaultTypeface() {
    if (!isMiui()) {
      return null;
    }
    if (sIsTypefaceInit) {
      return sDefaultTypeface;
    }
    try {
      TextPaint paint = new TextView(LynxEnv.inst().getAppContext()).getPaint();
      if (paint != null) {
        sDefaultTypeface = paint.getTypeface();
      }
    } catch (Exception e) {
      LLog.e("Lynx", "get default typeface failed");
    }
    sIsTypefaceInit = true;
    return sDefaultTypeface;
  }

  public static boolean isHuaWei() {
    return "HUAWEI".equals(Build.MANUFACTURER);
  }

  public static boolean isMeizu() {
    String brand = Build.BRAND;
    if (brand == null) {
      return false;
    }
    return brand.toLowerCase(Locale.ENGLISH).indexOf("meizu") > -1;
  }

  public static boolean isMeizu15() {
    if (isMeizu() && !TextUtils.isEmpty(Build.DEVICE)) {
      return Build.DEVICE.contains("15");
    }
    return false;
  }

  /**
   * after API 21, use {Build#SUPPORTED_ABIS} to get APIs, like: [arm64-v8a, armeabi-v7a, armeabi]
   * otherwise, use {Build#CPU_ABI} instead
   *
   * @return supported ABIs string, split by comma
   */
  private static String getCpuAbi() {
    if (sCpuABI == null) {
      try {
        StringBuilder abi = new StringBuilder();
        if (Build.VERSION.SDK_INT >= 21 && Build.SUPPORTED_ABIS.length > 0) {
          for (int i = 0; i < Build.SUPPORTED_ABIS.length; i++) {
            abi.append(Build.SUPPORTED_ABIS[i]);
            if (i != Build.SUPPORTED_ABIS.length - 1) {
              abi.append(", ");
            }
          }
        } else {
          abi = new StringBuilder(Build.CPU_ABI);
        }
        if (TextUtils.isEmpty(abi.toString())) {
          sCpuABI = UNKNOWN_CPU_ABI;
        }
        sCpuABI = abi.toString();
      } catch (Exception e) {
        LLog.e(TAG, "Lynx get unknown CPU ABIs");
        sCpuABI = UNKNOWN_CPU_ABI;
      }
    }
    return sCpuABI;
  }

  @CalledByNative
  public static boolean is64BitDevice() {
    if (sIs64Bit == DEVICE_ARCH_TYPE_UNDEFINED) {
      sIs64Bit = getCpuAbi().contains("64") ? DEVICE_ARCH_TYPE_64BIT : DEVICE_ARCH_TYPE_32IT;
    }
    return sIs64Bit == DEVICE_ARCH_TYPE_64BIT;
  }

  /**
   * Gets the refresh rate of this display in frames per second.
   * @param context lynx context
   * @return
   */
  public static float getRefreshRate(LynxContext context) {
    if (context == null) {
      return DEFAULT_DEVICE_REFRESH_RATE;
    }
    return getRefreshRate(context.getContext());
  }

  /**
   * Gets the refresh rate of this display in frames per second.
   * @param context If the Android version is less than 6.0, the context must be an Activity.
   * @return
   */
  public static float getRefreshRate(Context context) {
    Display display = null;
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
      DisplayManager manager = context.getSystemService(DisplayManager.class);
      if (manager != null) {
        display = manager.getDisplay(DEFAULT_DISPLAY);
      }
    } else {
      Activity activity = ContextUtils.getActivity(context);
      if (activity != null) {
        display = activity.getWindowManager().getDefaultDisplay();
      }
    }
    if (display != null) {
      return display.getRefreshRate();
    }
    return DEFAULT_DEVICE_REFRESH_RATE;
  }
}

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.RenderNode;
import android.graphics.Shader;
import android.os.Build;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RSRuntimeException;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicBlur;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.RequiresApi;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

@Keep
public class BlurUtils {
  private static final int BLUR_DEFAULT_SAMPLING = 1;
  private static final int DEFAULT_ITERATIONS = 3;
  // fixme(renzhongyue): change reflection to api level annotation after compileSdkVersion upgrades
  // to 31 or higher.
  private static Method sCreateBlurEffect = null;
  private static Method sSetRenderEffect = null;
  private static Method sSetNodeRenderEffect = null;
  private static Class sRenderEffectClass = null;
  private static boolean sSupportRenderEffect = true;

  private static final String TAG = "BlurUtils";

  /**
   * Set BlurEffect to the target View by reflect calling {@link View#setRenderEffect}, which needs
   * api level 31, but current compileSdkVersion is 29.
   * @param v target view
   * @param radius blur radius
   * @return true if the {@link android.graphics.RenderEffect} is set successfully.
   */
  public static boolean createEffect(View v, float radius) {
    if (!isSupportRenderEffect()) {
      return false;
    }

    // if radius<=0 no need to invoke blur method
    if (radius <= 0) {
      return false;
    }

    if (!prepareViewSetMethod()) {
      sSupportRenderEffect = false;
      return false;
    }

    try {
      Object blurEffect = sCreateBlurEffect.invoke(null, radius, radius, Shader.TileMode.CLAMP);
      sSetRenderEffect.invoke(v, blurEffect);
      return true;
    } catch (IllegalAccessException | IllegalArgumentException | InvocationTargetException e) {
      LLog.e(TAG, "createViewEffect failed " + e.getMessage());
      sSupportRenderEffect = false;
      return false;
    }
  }

  public static boolean createEffect(RenderNode node, float radius) {
    if (!isSupportRenderEffect()) {
      return false;
    }

    // if radius<=0 no need to invoke blur method
    if (radius <= 0) {
      return false;
    }

    if (!prepareRenderNodeSetMethod()) {
      sSupportRenderEffect = false;
      return false;
    }

    try {
      Object blurEffect = sCreateBlurEffect.invoke(null, radius, radius, Shader.TileMode.CLAMP);
      sSetNodeRenderEffect.invoke(node, blurEffect);
      return true;
    } catch (IllegalAccessException | IllegalArgumentException | InvocationTargetException e) {
      LLog.e(TAG, "createNodeEffect failed " + e.getMessage());
      sSupportRenderEffect = false;
      return false;
    }
  }

  private static boolean isSupportRenderEffect() {
    return Build.VERSION.SDK_INT >= 31 && sSupportRenderEffect;
  }

  /**
   * Set RenderEffect to null, remove blur effect on view.
   * @param v target view
   * @return true if succeed in removing RenderEffect
   */
  public static boolean removeEffect(View v) {
    if (!isSupportRenderEffect()) {
      return false;
    }

    if (!prepareViewSetMethod()) {
      sSupportRenderEffect = false;
      return false;
    }

    try {
      sSetRenderEffect.invoke(v, new Object[] {null});
      return true;
    } catch (IllegalAccessException | IllegalArgumentException | InvocationTargetException e) {
      sSupportRenderEffect = false;
      return false;
    }
  }

  private static boolean prepareRenderEffect() {
    if (sRenderEffectClass == null) {
      try {
        sRenderEffectClass = Class.forName("android.graphics.RenderEffect");
      } catch (ClassNotFoundException e) {
        return false;
      }
    }
    if (sRenderEffectClass != null && sCreateBlurEffect == null) {
      try {
        sCreateBlurEffect = sRenderEffectClass.getMethod("createBlurEffect",
            float.class /* radiusX */, float.class /* radiusY */, Shader.TileMode.class);
      } catch (NoSuchMethodException e) {
        LLog.e(TAG, "prepareRenderEffectClass failed");
        return false;
      }
    }
    if (sRenderEffectClass != null && sCreateBlurEffect != null) {
      return true;
    }
    return false;
  }

  private static boolean prepareViewSetMethod() {
    if (!prepareRenderEffect()) {
      return false;
    }
    if (sSetRenderEffect == null) {
      try {
        sSetRenderEffect = View.class.getMethod("setRenderEffect", sRenderEffectClass);
      } catch (NoSuchMethodException e) {
        LLog.e(TAG, "prepareRenderEffectMethods failed");
        return false;
      }
    }
    return true;
  }

  @RequiresApi(api = Build.VERSION_CODES.Q)
  private static boolean prepareRenderNodeSetMethod() {
    if (!prepareRenderEffect()) {
      return false;
    }
    if (sSetNodeRenderEffect == null) {
      try {
        sSetNodeRenderEffect = RenderNode.class.getMethod("setRenderEffect", sRenderEffectClass);
      } catch (NoSuchMethodException e) {
        LLog.e(TAG, "prepareRenderNodeRenderEffectMethods failed");
        sSupportRenderEffect = false;
        return false;
      }
    }
    return true;
  }

  /**
   * Blur the target bitmap with {@link ScriptIntrinsicBlur} first, if failed, blur the target
   * bitmap via a native implemented blur function.
   * @param context context, required by renderscript.
   * @param bitmap bitmap to blur
   * @return blurred bitmap.
   */
  @RequiresApi(17)
  public static Bitmap blur(
      Context context, Bitmap bitmap, int width, int height, float radius, int sampling) {
    if (width == 0 || height == 0) {
      return null;
    }
    Bitmap scaled = bitmap;
    Boolean doSampling = sampling > 1;
    if (doSampling) {
      // Create a sampled image, with smaller size.
      scaled = Bitmap.createScaledBitmap(bitmap, width / sampling, height / sampling, true);
    }
    try {
      // render script support max radius is 25. Over 25 will throw RSRuntimeException.
      scaled = rs(context, scaled, radius);
    } catch (RSRuntimeException e) {
      iterativeBoxBlur(scaled, (int) radius);
    }

    if (doSampling) {
      int pixels[] = new int[width * height];
      scaled = Bitmap.createScaledBitmap(scaled, width, height, true);
      scaled.getPixels(pixels, 0, width, 0, 0, width, height);
      bitmap.setPixels(pixels, 0, width, 0, 0, width, height);
    }
    return bitmap;
  }

  /**
   * Blur the bitmap via renderscript.
   * @param context context
   * @param bitmap origin bitmap to be blurred.
   * @param radius blur radius.
   * @return blurred bitmap
   * @throws RSRuntimeException Exception when executing the renderscript.
   */
  @RequiresApi(17)
  private static Bitmap rs(Context context, Bitmap bitmap, float radius) throws RSRuntimeException {
    RenderScript rs = null;
    Allocation input = null;
    Allocation output = null;
    ScriptIntrinsicBlur blur = null;
    try {
      rs = RenderScript.create(context);
      rs.setMessageHandler(new RenderScript.RSMessageHandler());
      input = Allocation.createFromBitmap(
          rs, bitmap, Allocation.MipmapControl.MIPMAP_NONE, Allocation.USAGE_SCRIPT);
      output = Allocation.createTyped(rs, input.getType());
      blur = ScriptIntrinsicBlur.create(rs, Element.U8_4(rs));
      blur.setInput(input);
      blur.setRadius(radius);
      blur.forEach(output);
      output.copyTo(bitmap);
    } finally {
      if (rs != null) {
        rs.destroy();
      }
      if (input != null) {
        input.destroy();
      }
      if (output != null) {
        output.destroy();
      }
      if (blur != null) {
        blur.destroy();
      }
    }
    return bitmap;
  }

  public static void iterativeBoxBlur(Bitmap bitmap, int blurRadius) {
    if (bitmap == null) {
      LLog.w("Blur", "bitmap is null");
      return;
    }
    if (blurRadius <= 0) {
      LLog.w("Blur", "radius <= 0");
      return;
    }

    try {
      TraceEvent.beginSection("image.BlurUtils.nativeIterativeBoxBlur");
      nativeIterativeBoxBlur(bitmap, DEFAULT_ITERATIONS, blurRadius);
      TraceEvent.endSection("image.BlurUtils.nativeIterativeBoxBlur");
    } catch (RuntimeException e) {
      LLog.w("Blur", e.getMessage());
    }
  }

  private static native void nativeIterativeBoxBlur(Bitmap bitmap, int iterations, int blurRadius);
}

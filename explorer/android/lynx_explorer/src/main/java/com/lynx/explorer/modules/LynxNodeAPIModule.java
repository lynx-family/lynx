// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.util.Log;
import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.tasm.behavior.LynxContext;
import java.util.HashMap;
import java.util.Map;

public class LynxNodeAPIModule extends LynxContextModule {
  private static final String TAG = "LynxNodeAPIModule";
  private static volatile boolean sNativeAvailable = false;

  static {
    try {
      // Prefer host/engine integrated native loader (e.g. SoLoader) if available.
      // Fall back to System.loadLibrary when no loader hook is provided.
      com.lynx.tasm.LynxEnv env = com.lynx.tasm.LynxEnv.inst();
      Object loader = (env != null) ? env.getLibraryLoader() : null;
      if (loader instanceof com.lynx.tasm.INativeLibraryLoader) {
        ((com.lynx.tasm.INativeLibraryLoader) loader).loadLibrary("lynx_napi_addon_loader");
      } else {
        System.loadLibrary("lynx_napi_addon_loader");
      }
      sNativeAvailable = true;
    } catch (UnsatisfiedLinkError e) {
      Log.e(TAG, "Failed to load native library: lynx_napi_addon_loader", e);
    }
  }

  private static final Map<Long, Long> sRuntimeEnvMap =
      java.util.Collections.synchronizedMap(new HashMap<>());

  public LynxNodeAPIModule(LynxContext context, Object param) {
    super(context, param);
  }

  public static void putEnv(LynxContext token, long napiEnv) {
    if (token != null && napiEnv != 0) {
      Long runtimeId = token.getRuntimeId();
      if (runtimeId != null) {
        sRuntimeEnvMap.put(runtimeId, napiEnv);
      }
    }
  }

  public static void removeEnv(LynxContext token) {
    if (token != null) {
      Long runtimeId = token.getRuntimeId();
      if (runtimeId != null) {
        sRuntimeEnvMap.remove(runtimeId);
      }
    }
  }

  @LynxMethod
  public void requireNodeAddon(String addonName) {
    if (!sNativeAvailable) {
      Log.w(TAG, "Native addon loader unavailable; ignore requireNodeAddon: " + addonName);
      return;
    }

    if (mLynxContext != null) {
      Long runtimeId = mLynxContext.getRuntimeId();
      if (runtimeId == null) {
        Log.w(TAG,
            "requireNodeAddon failed: runtimeId is null. Ensure enable_napi_addon is enabled and runtime attach callback has been received. addonName="
                + addonName);
        return;
      }
      Long napiEnv = sRuntimeEnvMap.get(runtimeId);
      if (napiEnv != null && napiEnv != 0) {
        nativeRequireNodeAddon(napiEnv, addonName);
      } else {
        Log.w(TAG,
            "requireNodeAddon failed: napiEnv missing/invalid for runtimeId=" + runtimeId
                + ". Ensure enable_napi_addon is enabled and runtime attach callback has been received. addonName="
                + addonName);
      }
    } else {
      Log.w(TAG,
          "requireNodeAddon failed: mLynxContext is null. Ensure module is initialized with a valid LynxContext. addonName="
              + addonName);
    }
  }

  private native void nativeRequireNodeAddon(long napiEnv, String addonName);
}

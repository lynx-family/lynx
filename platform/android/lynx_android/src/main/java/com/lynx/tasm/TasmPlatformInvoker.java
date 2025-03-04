// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class TasmPlatformInvoker {
  private static final String TAG = "TasmPlatformInvoker";

  // TODO(heshan): invoke LynxTemplateRender instead.
  private final WeakReference<NativeFacade> mNativeFacade;

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public TasmPlatformInvoker(NativeFacade nativeFacade) {
    mNativeFacade = new WeakReference<>(nativeFacade);
  }

  @CalledByNative
  private void onPageConfigDecoded(final ReadableMap map) {
    NativeFacade nativeFacade = mNativeFacade.get();
    if (nativeFacade == null) {
      LLog.i(TAG, "onPageConfigDecoded failed, NativeFacade has been released.");
      return;
    }

    nativeFacade.onPageConfigDecoded(map);
  }

  @CalledByNative
  private String translateResourceForTheme(final String resId, final String themeKey) {
    NativeFacade nativeFacade = mNativeFacade.get();
    if (nativeFacade == null) {
      LLog.i(TAG, "translateResourceForTheme failed, NativeFacade has been released.");
      return null;
    }

    return nativeFacade.translateResourceForTheme(resId, themeKey);
  }

  @CalledByNative
  private ByteBuffer triggerLepusBridge(final String methodName, final Object args) {
    NativeFacade nativeFacade = mNativeFacade.get();
    if (nativeFacade == null) {
      LLog.i(TAG, "triggerLepusBridge failed, NativeFacade has been released.");
      return null;
    }

    return nativeFacade.triggerLepusBridge(methodName, args);
  }

  @CalledByNative
  private void triggerLepusBridgeAsync(String methodName, Object args) {
    NativeFacade nativeFacade = mNativeFacade.get();
    if (nativeFacade == null) {
      LLog.i(TAG, "triggerLepusBridgeAsync failed, NativeFacade has been released.");
      return;
    }
    nativeFacade.triggerLepusBridgeAsync(methodName, args);
  }

  @CalledByNative
  private void getI18nResourceByNative(final String channel, final String fallbackUrl) {
    NativeFacade nativeFacade = mNativeFacade.get();
    if (nativeFacade == null) {
      LLog.i(TAG, "getI18nResourceByNative failed, NativeFacade has been released.");
      return;
    }

    nativeFacade.getI18nResourceByNative(channel, fallbackUrl);
  }
}

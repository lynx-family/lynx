// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.text.TextUtils;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.TemplateBundleOption;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.service.security.ILynxSecurityService;
import com.lynx.tasm.service.security.SecurityResult;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

/**
 * Class to decode `template.js` in Prev, and can get some Info in the `Template.js` file.
 */
public final class TemplateBundle {
  private static final String TEMPLATE_BUNDLE_FROM_TEMPLATE = "TemplateBundle.fromTemplate";

  private long nativePtr = 0; // native pointer for LynxTemplateBundle.
  private Map<String, Object> extraInfo;
  private String errorMsg = null;
  private int templateSize;

  private TemplateBundle(long ptr, int templateSize, String errMsg) {
    this.nativePtr = ptr;
    this.errorMsg = errMsg;
    this.templateSize = templateSize;
  }

  public static TemplateBundle fromTemplate(byte[] template) {
    TemplateBundle result = null;
    TraceEvent.beginSection(TEMPLATE_BUNDLE_FROM_TEMPLATE);
    if (template != null) {
      if (checkIfEnvPrepared()) {
        ILynxSecurityService securityService =
            LynxServiceCenter.inst().getService(ILynxSecurityService.class);
        if (securityService != null) {
          // Do Security Check;
          SecurityResult securityResult = securityService.verifyTASM(
              null, template, null, ILynxSecurityService.LynxTasmType.TYPE_TEMPLATE);
          if (!securityResult.isVerified()) {
            result = new TemplateBundle(0, template.length,
                "template verify failed, error message: " + securityResult.getErrorMsg());
            return result;
          }
        }
        String[] buffer = new String[1];
        long ptr = nativeParseTemplate(template, buffer);
        result = new TemplateBundle(ptr, template.length, buffer[0]);
      } else {
        result = new TemplateBundle(0, template.length, "Lynx Env is not prepared");
      }

      TraceEvent.endSection(TEMPLATE_BUNDLE_FROM_TEMPLATE);
    }
    return result;
  }

  public static TemplateBundle fromTemplate(byte[] template, TemplateBundleOption option) {
    TemplateBundle result = TemplateBundle.fromTemplate(template);
    result.initWithOption(option);
    return result;
  }

  @CalledByNative
  private static TemplateBundle fromNative(long nativePtr) {
    // TODO(nihao.royal) add template size for recycled TemplateBundle.
    String errMsg = nativePtr == 0 ? "native TemplateBundle doesn't exist" : null;
    return new TemplateBundle(nativePtr, 0, errMsg);
  }

  private void initWithOption(TemplateBundleOption option) {
    if (!isValid() || option == null) {
      return;
    }
    nativeInitWithOption(
        nativePtr, option.getContextPoolSize(), option.getEnableContextAutoRefill());
  }

  /**
   * get ExtraInfo of a template.js
   *
   * @return ExtraInfo of LynxTemplate
   */
  public Map<String, Object> getExtraInfo() {
    if (extraInfo == null) {
      if (checkIfEnvPrepared() && isValid()) {
        extraInfo = new HashMap<>();
        Object data = nativeGetExtraInfo(nativePtr);
        if (data instanceof Map) {
          extraInfo.putAll((Map<String, Object>) data);
        }
      }
    }
    return extraInfo;
  }

  /**
   * Check if the ElementBundle contained in TemplateBundle is Valid.
   * @return state of ElementBundle
   */
  public boolean isElementBundleValid() {
    boolean valid = false;
    if (checkIfEnvPrepared() && isValid()) {
      valid = nativeGetContainsElementTree(nativePtr);
    }
    return valid;
  }

  public int getTemplateSize() {
    return this.templateSize;
  }

  /**
   *
   * @return Native ptr of TemplateBundle
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public long getNativePtr() {
    return this.nativePtr;
  }

  /**
   * Release The TemplateBundle to avoid mem leak.
   */
  public void release() {
    if (checkIfEnvPrepared() && nativePtr != 0) {
      nativeReleaseBundle(nativePtr);
      nativePtr = 0;
    }
  }

  @Override
  protected void finalize() throws Throwable {
    release();
  }

  public boolean isValid() {
    return nativePtr != 0;
  }

  private static boolean checkIfEnvPrepared() {
    return LynxEnv.inst().isNativeLibraryLoaded();
  }

  @CalledByNative
  private static Object decodeByteBuffer(ByteBuffer buffer) {
    if (buffer != null) {
      return LepusBuffer.INSTANCE.decodeMessage(buffer);
    }
    return null;
  }

  /**
   * Post a task to generate bytecode for a given template bundle.
   * The task will be executed in a background thread.
   * @param bytecodeSourceUrl The source url of the template.
   * @param useV8 Whether to generate bytecode for V8 engine instead of QuickJS.
   */
  public void postJsCacheGenerationTask(String bytecodeSourceUrl, boolean useV8) {
    if (!isValid() || TextUtils.isEmpty(bytecodeSourceUrl)) {
      return;
    }
    nativePostJsCacheGenerationTask(getNativePtr(), bytecodeSourceUrl, useV8);
  }

  /**
   * Return the error message of parsing template.
   * @return error message. If the bundle is valid, the return value will be null.
   */
  public String getErrorMessage() {
    return errorMsg;
  }

  /**
   * Deprecated now, please use TemplateBundleOption
   */
  @Deprecated
  public boolean constructContext(int count) {
    return checkIfEnvPrepared() && isValid() && nativeConstructContext(nativePtr, count);
  }

  /**
   * Deprecated now, please use TemplateBundleOption
   */
  @Deprecated
  public boolean constructContext() {
    return constructContext(1);
  }

  private static native void nativePostJsCacheGenerationTask(
      long bundle, String bytecodeSourceUrl, boolean useV8);

  private static native long nativeParseTemplate(byte[] temp, String[] buffer);
  private static native void nativeReleaseBundle(long ptr);
  private static native Object nativeGetExtraInfo(long ptr);
  private static native boolean nativeGetContainsElementTree(long ptr);

  private static native boolean nativeConstructContext(long ptr, int count);

  private static native void nativeInitWithOption(
      long ptr, int contextPoolSize, boolean enableContextAutoRefill);
}

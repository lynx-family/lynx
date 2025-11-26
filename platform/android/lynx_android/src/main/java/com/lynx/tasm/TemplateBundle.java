// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.text.TextUtils;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.jsbridge.LynxBytecodeCallback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.service.security.ILynxSecurityService;
import com.lynx.tasm.service.security.SecurityResult;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

/**
 * @apidoc
 * @brief `TemplateBundle` is the output product of the PreDecode capability
 * provided by the Lynx SDK. Client developers can parse the Lynx App Bundle product
 * in advance to obtain the `TemplateBundle` object and consume the App Bundle product.
 */
public final class TemplateBundle {
  public static final String TAG = "TemplateBundle";

  private final String url;
  private long nativePtr = 0; // native pointer for LynxTemplateBundle.
  private Map<String, Object> extraInfo;
  private String errorMsg = null;
  private final int templateSize;

  private final PageConfig pageConfig;
  private OnReleaseCallback onReleaseCallback;

  interface OnReleaseCallback {
    void onRelease();
  }
  private TemplateBundle(
      long ptr, int templateSize, String url, String errMsg, ReadableMap pageConfigMap) {
    this.nativePtr = ptr;
    this.templateSize = templateSize;
    this.url = url;
    this.errorMsg = errMsg;
    this.pageConfig = new PageConfig(pageConfigMap);
  }

  @Override
  public boolean equals(Object o) {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    TemplateBundle that = (TemplateBundle) o;
    return nativePtr == that.nativePtr && templateSize == that.templateSize;
  }

  private static SecurityResult verifyTasm(byte[] template, ByteBuffer buffer, String url) {
    ILynxSecurityService securityService =
        LynxServiceCenter.inst().getService(ILynxSecurityService.class);
    if (securityService != null) {
      // Do Security Check;
      return securityService.verifyTASM(
          null, template, buffer, url, ILynxSecurityService.LynxTasmType.TYPE_TEMPLATE);
    }
    return SecurityResult.onSuccess();
  }

  private static TemplateBundle internalBuildTemplate(
      byte[] template, ByteBuffer buffer, String url) {
    TemplateBundle result = null;
    int length;
    if (template != null || buffer != null) {
      length = buffer != null ? buffer.limit() : template.length;
      TraceEvent.beginSection(TraceEventDef.TEMPLATE_BUNDLE_FROM_TEMPLATE);
      if (checkIfEnvPrepared()) {
        SecurityResult securityResult = verifyTasm(template, buffer, url);
        if (!securityResult.isVerified()) {
          result = new TemplateBundle(0, template.length, url,
              "template verify failed, error message: " + securityResult.getErrorMsg(), null);
          return result;
        }
        // 0: string, error message
        // 1: ReadableMap, pageConfig
        Object[] options = new Object[2];
        long ptr = 0;
        if (buffer != null) {
          ptr = nativeParseTemplateFromByteBuffer(buffer, options);
        } else {
          ptr = nativeParseTemplateFromByteArray(template, options);
        }
        result =
            new TemplateBundle(ptr, length, url, (String) options[0], (ReadableMap) options[1]);
      } else {
        result = new TemplateBundle(0, length, url, "Lynx Env is not prepared", null);
      }
      TraceEvent.endSection(TraceEventDef.TEMPLATE_BUNDLE_FROM_TEMPLATE);
    }
    return result;
  }

  /**
   * @apidoc
   * @brief Input Lynx Bundle binary content and return the parsed `TemplateBundle` object.
   * @param template Template binary content.
   * @return The `TemplateBundle` object.
   * @note When the input `template` is `null`, this method returns `null` directly.
   * @note When the input `template` is not a correct `Lynx` template data,
   * an invalid `TemplateBundle` is returned.
   */
  public static TemplateBundle fromTemplate(byte[] template) {
    return internalBuildTemplate(template, null, null);
  }

  /**
   * @apidoc
   * @brief Input ByteBuffer that represents Lynx Bundle content and return the parsed
   * `TemplateBundle` object.
   * @param buffer ByteBuffer that represents Lynx Bundle content
   * @return The `TemplateBundle` object.
   * @note When the input `buffer` is `null`, this method returns `null` directly.
   * @note When the input `buffer` is not `DirectByteBuffer`, returns an invalid TemplateBundle.
   * @note When the input `template` is not a correct `Lynx` template data,
   * an invalid `TemplateBundle` is returned.
   */
  public static TemplateBundle fromTemplate(ByteBuffer buffer, TemplateBundleOption option) {
    String url = option != null ? option.getUrl() : null;
    if (buffer == null) {
      return null;
    }

    if (!buffer.isDirect()) {
      LLog.i(TAG, "TemplateBundle only supports DirectByteBuffer.");
      return new TemplateBundle(
          0, buffer.limit(), url, "TemplateBundle only supports DirectByteBuffer.", null);
    }
    TemplateBundle result = internalBuildTemplate(null, buffer, url);
    result.initWithOption(option);
    return result;
  }

  public static TemplateBundle fromTemplate(byte[] template, TemplateBundleOption option) {
    String url = option != null ? option.getUrl() : null;
    TemplateBundle result = internalBuildTemplate(template, null, url);
    result.initWithOption(option);
    return result;
  }

  @CalledByNative
  private static TemplateBundle fromNative(long nativePtr, ReadableMap pageConfigMap) {
    // TODO(nihao.royal) add template size & template url for recycled TemplateBundle.
    String errMsg = nativePtr == 0 ? "native TemplateBundle doesn't exist" : null;
    return new TemplateBundle(nativePtr, 0, null, errMsg, pageConfigMap);
  }

  private void initWithOption(TemplateBundleOption option) {
    if (!isValid() || option == null) {
      return;
    }
    nativeInitWithOption(
        nativePtr, option.getContextPoolSize(), option.getEnableContextAutoRefill());
  }

  /**
   * @apidoc
   * @brief Read the content of the `extraInfo` field configured in the `pageConfig` of the
   * front-end template.
   *
   * @return When the front-end does not configure `extraInfo` or is called on an empty
   *     `TemplateBundle` object,
   * it returns `null`, else it returns the `extraInfo` field.
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
   * @apidoc
   * @brief Whether the TemplateBundle contains a Valid ElementBundle.
   * @returns True if valid, else false.
   */
  public boolean isElementBundleValid() {
    boolean valid = false;
    if (checkIfEnvPrepared() && isValid()) {
      valid = nativeGetContainsElementTree(nativePtr);
    }
    return valid;
  }

  /**
   * Get the size of current template.
   * @apidoc
   * @returns Template size.
   */
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
   * @apidoc
   * @brief Release the `Native` memory held by the current `TemplateBundle` object.
   * After executing the `release` method, the `TemplateBundle` will become the `inValid` state.
   */
  public void release() {
    if (checkIfEnvPrepared() && nativePtr != 0) {
      if (onReleaseCallback != null) {
        onReleaseCallback.onRelease();
        onReleaseCallback = null;
      }
      nativeReleaseBundle(nativePtr);
      nativePtr = 0;
    }
  }

  @Override
  protected void finalize() throws Throwable {
    release();
  }

  /**
   * @apidoc
   * @brief Determines whether the current `TemplateBundle` object is valid.
   * @return `true` if the current `TemplateBundle` object is valid, else `false`.
   */
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
   * @apidoc
   * Post a task to generate bytecode for a given template bundle.
   * The task will be executed in a background thread.
   * @param bytecodeSourceUrl The source url of the template.
   * @param useV8 Whether to generate bytecode for V8 engine instead of QuickJS.
   * @param callback When generate finished, this will response the result.
   */
  public void postJsCacheGenerationTask(
      String bytecodeSourceUrl, boolean useV8, @Nullable LynxBytecodeCallback callback) {
    if (!isValid() || TextUtils.isEmpty(bytecodeSourceUrl)) {
      return;
    }
    nativePostJsCacheGenerationTask(getNativePtr(), bytecodeSourceUrl, useV8, callback);
  }

  /**
   * @apidoc
   * @brief Start a sub-thread task to generate the `js code cache` of the current template.
   * @param bytecodeSourceUrl The `url` of the current template.
   * @param useV8 Whether to generate `V8 Code Cache`, otherwise generate `QuickJS Code Cache`.
   */
  public void postJsCacheGenerationTask(String bytecodeSourceUrl, boolean useV8) {
    postJsCacheGenerationTask(bytecodeSourceUrl, useV8, null);
  }

  /**
   * @apidoc
   * @brief When `TemplateBundle` is an invalid object, use this method to
   * obtain the exception information that occurred during template parsing
   * @return The exception information.
   */
  public String getErrorMessage() {
    return errorMsg;
  }

  /**
   * Return the sourceUrl of passing template.
   * @return sourceUrl
   */
  public String getUrl() {
    return url;
  }

  /**
   * Deprecated now, please use TemplateBundleOption
   */
  @Deprecated
  public boolean constructContext(int count) {
    return checkIfEnvPrepared() && isValid() && nativeConstructContext(nativePtr, count);
  }

  /**
   * Get PageConfig from TemplateBundle
   * @return PageConfig
   */
  @RestrictTo({RestrictTo.Scope.LIBRARY})
  public PageConfig getPageConfig() {
    return pageConfig;
  }

  /**
   * Deprecated now, please use TemplateBundleOption
   */
  @Deprecated
  public boolean constructContext() {
    return constructContext(1);
  }

  void setOnReleaseCallback(OnReleaseCallback callback) {
    onReleaseCallback = callback;
  }
  private static native void nativePostJsCacheGenerationTask(
      long bundle, String bytecodeSourceUrl, boolean useV8, LynxBytecodeCallback callback);

  private static native long nativeParseTemplateFromByteArray(byte[] temp, Object[] options);
  private static native long nativeParseTemplateFromByteBuffer(ByteBuffer bytes, Object[] options);
  private static native void nativeReleaseBundle(long ptr);
  private static native Object nativeGetExtraInfo(long ptr);
  private static native boolean nativeGetContainsElementTree(long ptr);

  private static native boolean nativeConstructContext(long ptr, int count);

  private static native void nativeInitWithOption(
      long ptr, int contextPoolSize, boolean enableContextAutoRefill);
}

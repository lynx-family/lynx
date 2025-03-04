// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.react.bridge;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.common.LepusBuffer;
import java.nio.ByteBuffer;

public final class PiperData {
  public enum DataType { Empty, String, Map }

  private long mNativeDataPtr = 0;
  private ByteBuffer mBuffer = null;
  private DataType mType = DataType.Empty;
  // This flag indicates whether Lynx can immediately recycle PiperData after use. Once this flag is
  // set to true, PiperData can only be used once, please do not use it multiple times.
  private boolean mIsDisposable = false;
  private Object mRawData = null;

  @NonNull
  public static PiperData fromString(String piperData) {
    if (checkIfEnvPrepared() && !TextUtils.isEmpty(piperData)) {
      return new PiperData(piperData, false);
    } else {
      return new PiperData();
    }
  }

  @NonNull
  public static PiperData fromObject(Object data) {
    return new PiperData(data, false);
  }

  // Create a disposable piper data from string which will be recycled after use. A disposable piper
  // data can only be used once.
  @NonNull
  public static PiperData createDisposableFromString(String piperData) {
    if (checkIfEnvPrepared() && !TextUtils.isEmpty(piperData)) {
      return new PiperData(piperData, true);
    } else {
      return new PiperData();
    }
  }

  // Create a disposable piper data from object which will be recycled after use. A disposable piper
  // data can only be used once.
  @NonNull
  public static PiperData createDisposableFromObject(Object data) {
    return new PiperData(data, true);
  }

  private PiperData() {}

  private PiperData(String piperData, boolean isDisposable) {
    mType = DataType.String;
    mNativeDataPtr = nativeParseStringData(piperData);
    mIsDisposable = isDisposable;
    mRawData = piperData;
  }

  private PiperData(Object data, boolean isDisposable) {
    mType = DataType.Map;
    mBuffer = LepusBuffer.INSTANCE.encodeMessage(data);
    mIsDisposable = isDisposable;
    mRawData = data;
  }

  // This API is used to retrieve the raw data (such as JSONObject, String, etc.) that was passed in
  // when creating PiperData. The return type is 'Object'. It is recommended to use 'instanceof' to
  // determine the actual type of the return value before using it.
  public Object getRawData() {
    return mRawData;
  }

  public boolean hasParseError() {
    return (mType == DataType.String && mNativeDataPtr == 0);
  }

  // Mark PiperData disposable and Lynx can immediately recycle PiperData after use. Once PiperData
  // is disposable, it can only be used once, please do not use it multiple times.
  public void markDisposable() {
    mIsDisposable = true;
  }

  @CalledByNative
  public long getNativePtr() {
    return mNativeDataPtr;
  }

  @CalledByNative
  public ByteBuffer getBuffer() {
    return mBuffer;
  }

  @CalledByNative
  public int getBufferPosition() {
    if (mBuffer == null) {
      return 0;
    }
    return mBuffer.position();
  }

  public boolean isDisposable() {
    return mIsDisposable;
  }

  @CalledByNative
  public int getDataType() {
    return mType.ordinal();
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    if (mType != DataType.Empty) {
      recycle();
    }
  }

  @CalledByNative
  private boolean recycleIfIsDisposable() {
    if (mIsDisposable && mType != DataType.Empty) {
      recycle();
    }
    return mIsDisposable;
  }

  private void recycle() {
    if (mNativeDataPtr != 0 && checkIfEnvPrepared()) {
      nativeReleaseData(mNativeDataPtr);
      mNativeDataPtr = 0;
    }
    mType = DataType.Empty;
    mBuffer = null;
  }

  private static boolean checkIfEnvPrepared() {
    return LynxEnv.inst().isNativeLibraryLoaded();
  }

  private static native long nativeParseStringData(String data);
  private static native void nativeReleaseData(long data);
}

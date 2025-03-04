// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

/**
 * Standard Lynx Resource Response
 * @param <T> generic Type for Response, default Object
 */
public class LynxResourceResponse<T> {
  // error code should be negative number
  public static final int FAILED = -1;
  public static final int SUCCESS = 0;
  private Throwable mError;
  private int mCode;
  private T mData;

  public Throwable getError() {
    return mError;
  }

  private LynxResourceResponse(int code, Throwable error) {
    this.mCode = code;
    mError = error;
  }

  private LynxResourceResponse(T data) {
    this.mData = data;
  }

  public static LynxResourceResponse failed(int code, Throwable error) {
    return new LynxResourceResponse(code, error);
  }

  public static <T> LynxResourceResponse<T> success(T data) {
    LynxResourceResponse<T> response = new LynxResourceResponse<T>(data);
    response.mCode = SUCCESS;
    return response;
  }

  public T getData() {
    return mData;
  }

  public boolean success() {
    return mData != null;
  }

  public int getCode() {
    return mCode;
  }
}

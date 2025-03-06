// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider;

public final class LynxResourceResponse<T> {
  public enum ResponseState { FAILED, SUCCESS }

  private ResponseState state;
  private T data;
  private Throwable error;

  private LynxResourceResponse() {}

  public ResponseState getState() {
    return this.state;
  }

  public Throwable getError() {
    return this.error;
  }

  public T getData() {
    return this.data;
  }

  public static <T> LynxResourceResponse<T> onFailed(Throwable error) {
    LynxResourceResponse<T> response = new LynxResourceResponse<>();
    response.state = ResponseState.FAILED;
    response.error = error;
    return response;
  }

  public static <T> LynxResourceResponse<T> onSuccess(T data) {
    LynxResourceResponse<T> response = new LynxResourceResponse<T>();
    response.state = ResponseState.SUCCESS;
    response.data = data;
    return response;
  }
}

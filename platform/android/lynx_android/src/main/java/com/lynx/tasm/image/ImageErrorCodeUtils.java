// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.BindException;
import java.net.ConnectException;
import java.net.NoRouteToHostException;
import java.net.PortUnreachableException;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import org.apache.http.conn.ConnectTimeoutException;

public class ImageErrorCodeUtils {
  public static final String LYNX_IMAGE_ERROR_CODE_KEY = "error_code";
  public static final String LYNX_IMAGE_CATEGORIZED_CODE_KEY = "lynx_categorized_code";

  // error code
  // -1 UNDEFINED Error
  public static final int LYNX_IMAGE_UNKNOWN_EXCEPTION = -1;

  // [1000,1100) USER_OR_DESIGN Error
  public static final int LYNX_IMAGE_USER_OR_DESIGN_INTERVAL_START = 1000;
  public static final int LYNX_IMAGE_USER_OR_DESIGN_INTERVAL_END = 1100;
  public static final int LYNX_IMAGE_STATUS_CANCEL = 1001;
  public static final int LYNX_IMAGE_NETWORK_NOT_AVAILABLE = 1002;

  // [1100,1200) NET Error
  public static final int LYNX_IMAGE_TTNET_INTERVAL_START = 1100;
  public static final int LYNX_IMAGE_TTNET_INTERVAL_END = 1200;
  public static final int LYNX_IMAGE_CONNECT_TIMEOUT_EXCEPTION = 1101;
  public static final int LYNX_IMAGE_SOCKET_TIMEOUT_EXCEPTION = 1102;
  public static final int LYNX_IMAGE_CONNECT_EXCEPTION = 1103;
  public static final int LYNX_IMAGE_BIND_EXCEPTION = 1104;
  public static final int LYNX_IMAGE_NO_ROUTE_TO_HOST_EXCEPTION = 1105;
  public static final int LYNX_IMAGE_PORT_UNREACHABLE_EXCEPTION = 1106;
  public static final int LYNX_IMAGE_UNKNOWN_HOST_EXCEPTION = 1107;
  public static final int LYNX_IMAGE_SOCKET_EXCEPTION = 1108;

  // [1200,1300)  PIC_SOURCE Error
  public static final int LYNX_IMAGE_PIC_SOURCE_INTERVAL_START = 1200;
  public static final int LYNX_IMAGE_PIC_SOURCE_INTERVAL_END = 1300;
  public static final int LYNX_IMAGE_DECODE_ARGUMENT_EXCEPTION = 1201;
  public static final int LYNX_IMAGE_DECODE_STATE_EXCEPTION = 1202;
  public static final int LYNX_IMAGE_DECODE_RUNTIME_EXCEPTION = 1203;
  public static final int LYNX_IMAGE_THREAD_INTERRUPTED_EXCEPTION = 1204;
  public static final int LYNX_IMAGE_FILE_NOT_FOUND_EXCEPTION = 1205;
  public static final int LYNX_IMAGE_IO_EXCEPTION = 1206;

  public static int checkImageException(Throwable throwable) {
    int errorCode = LYNX_IMAGE_UNKNOWN_EXCEPTION;
    if (throwable == null) {
      return errorCode;
    }

    if (throwable instanceof ConnectTimeoutException) {
      errorCode = LYNX_IMAGE_CONNECT_TIMEOUT_EXCEPTION;
    } else if (throwable instanceof SocketTimeoutException) {
      errorCode = LYNX_IMAGE_SOCKET_TIMEOUT_EXCEPTION;
    } else if (throwable instanceof ConnectException) {
      errorCode = LYNX_IMAGE_CONNECT_EXCEPTION;
    } else if (throwable instanceof BindException) {
      errorCode = LYNX_IMAGE_BIND_EXCEPTION;
    } else if (throwable instanceof NoRouteToHostException) {
      errorCode = LYNX_IMAGE_NO_ROUTE_TO_HOST_EXCEPTION;
    } else if (throwable instanceof PortUnreachableException) {
      errorCode = LYNX_IMAGE_PORT_UNREACHABLE_EXCEPTION;
    } else if (throwable instanceof UnknownHostException) {
      errorCode = LYNX_IMAGE_UNKNOWN_HOST_EXCEPTION;
    } else if (throwable instanceof SocketException) {
      errorCode = LYNX_IMAGE_SOCKET_EXCEPTION;
    } else if (throwable instanceof IllegalArgumentException) {
      errorCode = LYNX_IMAGE_DECODE_ARGUMENT_EXCEPTION;
    } else if (throwable instanceof IllegalStateException) {
      errorCode = LYNX_IMAGE_DECODE_STATE_EXCEPTION;
    } else if (throwable instanceof RuntimeException) {
      errorCode = LYNX_IMAGE_DECODE_RUNTIME_EXCEPTION;
    } else if (throwable instanceof InterruptedException) {
      errorCode = LYNX_IMAGE_THREAD_INTERRUPTED_EXCEPTION;
    } else if (throwable instanceof FileNotFoundException) {
      errorCode = LYNX_IMAGE_FILE_NOT_FOUND_EXCEPTION;
    } else if (throwable instanceof IOException) {
      errorCode = LYNX_IMAGE_IO_EXCEPTION;
      String errorDesc = throwable.getMessage();
      if (errorDesc != null && errorDesc.length() != 0) {
        if (errorDesc.contains("canceled") || errorDesc.contains("Canceled")) {
          return LYNX_IMAGE_STATUS_CANCEL; // Cronet cancel or OkHttp3 cancel
        } else if (errorDesc.contains("network not available")) {
          return LYNX_IMAGE_NETWORK_NOT_AVAILABLE;
        }
      }
    }
    return errorCode;
  }

  public static int checkImageExceptionCategory(int errorCode) {
    int categoryCode = LynxSubErrorCode.E_RESOURCE_IMAGE_FROM_NETWORK_OR_OTHERS;
    if (errorCode >= LYNX_IMAGE_USER_OR_DESIGN_INTERVAL_START
        && errorCode < LYNX_IMAGE_USER_OR_DESIGN_INTERVAL_END) {
      categoryCode = LynxSubErrorCode.E_RESOURCE_IMAGE_FROM_USER_OR_DESIGN;
    } else if (errorCode >= LYNX_IMAGE_TTNET_INTERVAL_START
        && errorCode < LYNX_IMAGE_TTNET_INTERVAL_END) {
      categoryCode = LynxSubErrorCode.E_RESOURCE_IMAGE_FROM_NETWORK_OR_OTHERS;
    } else if (errorCode >= LYNX_IMAGE_PIC_SOURCE_INTERVAL_START
        && errorCode < LYNX_IMAGE_PIC_SOURCE_INTERVAL_END) {
      categoryCode = LynxSubErrorCode.E_RESOURCE_IMAGE_PIC_SOURCE;
    }
    return categoryCode;
  }
}

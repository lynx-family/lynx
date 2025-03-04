// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import android.text.TextUtils;
import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.ILynxResourceServiceResponse;
import com.lynx.tasm.service.LynxResourceServiceCallback;
import com.lynx.tasm.service.LynxResourceServiceRequestParams;
import com.lynx.tasm.service.LynxServiceCenter;

/**
 * Lynx Resource Service Provider
 */
public class LynxResourceServiceProvider<T> extends LynxResourceFetcher<T> {
  private final static String TAG = "LynxResourceServiceProvider";

  private static volatile boolean sInitialized = false;
  private static volatile ILynxResourceService resourceService = null;

  /**
   * Mark sure that LynxResourceService is registered.
   */
  public static synchronized boolean ensureLynxService() {
    if (!sInitialized) {
      resourceService = LynxServiceCenter.inst().getService(ILynxResourceService.class);
      sInitialized = true;
    }
    return resourceService == null;
  }

  /**
   * Send resource request with callback
   * @param request   request object, contain url and params and resource type.
   * @param callback  callback when resource finish
   */
  @AnyThread
  public ILynxResourceRequestOperation request(@NonNull final LynxResourceRequest<T> request,
      @NonNull final LynxResourceCallback<ILynxResourceResponseDataInfo> callback) {
    if (TextUtils.isEmpty(request.getUrl())) {
      callbackWithError(callback, "null",
          LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
          "The url in LynxResourceRequest is empty.");
      return null;
    }
    if (!LynxResourceServiceProvider.ensureLynxService()) {
      callbackWithError(callback, request.getUrl(),
          LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
          "Lynx resource service init failed");
      return null;
    }

    LynxResourceServiceRequestParams requestParams = null;
    if (request.getLynxResourceServiceRequestParams() != null) {
      requestParams = request.getLynxResourceServiceRequestParams();
    } else {
      requestParams = new LynxResourceServiceRequestParams();
    }

    /**
     * This method is not necessarily async, it may invoke callback sync in some specific cases,
     * such as local or cache exist
     * The invalid url will be detected by LynxResourceService, so the callback will always be
     * invoked correctly.
     */
    ILynxResourceRequestOperation operation = resourceService.fetchResourceAsync(
        request.getUrl(), requestParams, new LynxResourceServiceCallback() {
          @Override
          public void onResponse(@NonNull ILynxResourceServiceResponse response) {
            if (response != null && response.isSucceed()) {
              callback.onResponse(LynxResourceResponse.success(response));
              LLog.i(TAG,
                  "Lynx resource service fetchResourceAsync successful, the url is"
                      + request.getUrl());
            } else {
              /**
               * RESULT_EXCEPTION means an exception happened in lynx service, which means the
               * request should be retried with other providers
               */
              callbackWithError(callback, request.getUrl(),
                  LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
                  response == null ? "Lynx resource service response is null"
                                   : response.getErrorInfoString());
            }
          }
        });
    return operation;
  }

  /**
   * Send resource request sync
   * @param request   request object, contain url and params and resource type.
   */
  @NonNull
  public LynxResourceResponse<ILynxResourceResponseDataInfo> requestSync(
      @NonNull final LynxResourceRequest<T> request) {
    if (TextUtils.isEmpty(request.getUrl())) {
      return LynxResourceResponse.failed(
          LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
          new Throwable("The url in LynxResourceRequest is empty."));
    }
    if (resourceService == null) {
      LynxResourceResponse<ILynxResourceResponseDataInfo> response =
          LynxResourceResponse.failed(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
              new Throwable("Lynx resource service init failed"));
      return response;
    }

    LynxResourceServiceRequestParams requestParams = null;
    if (request.getLynxResourceServiceRequestParams() != null) {
      requestParams = request.getLynxResourceServiceRequestParams();
    } else {
      requestParams = new LynxResourceServiceRequestParams();
    }

    ILynxResourceServiceResponse resourceServiceResponse =
        resourceService.fetchResourceSync(request.getUrl(), requestParams);
    LynxResourceResponse<ILynxResourceResponseDataInfo> response = null;
    if (resourceServiceResponse == null) {
      response =
          LynxResourceResponse.failed(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
              new Throwable("Lynx resource service response is null"));
      LLog.e(TAG,
          "Lynx resource service request failed, the url is " + request.getUrl()
              + ", the error code is "
              + LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED
              + ", and the error message is Lynx resource service response is null.");

    } else if (resourceServiceResponse.isSucceed()) {
      response = LynxResourceResponse.success(resourceServiceResponse);
      LLog.i(
          TAG, "Lynx resource service fetchResourceSync successful, the url is" + request.getUrl());
    } else if (resourceServiceResponse.getErrorCode() == ILynxResourceService.RESULT_EXCEPTION) {
      response =
          LynxResourceResponse.failed(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
              new Throwable(resourceServiceResponse.getErrorInfoString()));
      LLog.e(TAG,
          "Lynx resource service request failed, the url is " + request.getUrl()
              + ", the error code is "
              + LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED
              + ", and the error message is " + resourceServiceResponse.getErrorInfoString());
    } else {
      response =
          LynxResourceResponse.failed(LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
              new Throwable(resourceServiceResponse.getErrorInfoString()));
      LLog.e(TAG,
          "Lynx resource service request failed, the url is " + request.getUrl()
              + ", the error code is "
              + LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED
              + ", and the error message is " + resourceServiceResponse.getErrorInfoString());
    }
    return response;
  }

  @AnyThread
  private void callbackWithError(
      @NonNull final LynxResourceCallback<ILynxResourceResponseDataInfo> callback, final String url,
      final int errorCode, @NonNull final String errorMsg) {
    LLog.e(TAG,
        "Lynx resource service request failed, the url is " + url + ", the error code is "
            + errorCode + ", and the error message is " + errorMsg);
    callback.onResponse(LynxResourceResponse.failed(errorCode, new Throwable(errorMsg)));
  }
}

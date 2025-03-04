// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import com.lynx.tasm.base.LLog;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.generic.StreamDelegate;
import java.io.IOException;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.Response;
import retrofit2.Retrofit;

public class DemoGenericResourceFetcher extends LynxGenericResourceFetcher {
  public static final String TAG = "DemoGenericResourceFetcher";

  @Override
  public void fetchResource(LynxResourceRequest request, LynxResourceCallback<byte[]> callback) {
    if (request == null) {
      callback.onResponse(LynxResourceResponse.onFailed(new Throwable("request is null!")));
      return;
    }

    LLog.i(TAG, "fetchResource: " + request.getUrl());
    Retrofit retrofit = new Retrofit.Builder().baseUrl("https://example.com/").build();
    TemplateApi templateApi = retrofit.create(TemplateApi.class);
    Call<ResponseBody> call = templateApi.getTemplate(request.getUrl());
    call.enqueue(new retrofit2.Callback<ResponseBody>() {
      @Override
      public void onResponse(Call<ResponseBody> call, Response<ResponseBody> response) {
        try {
          if (response.body() != null) {
            callback.onResponse(LynxResourceResponse.onSuccess(response.body().bytes()));
          } else {
            callback.onResponse(
                LynxResourceResponse.onFailed(new Throwable("response body is null.")));
          }
        } catch (IOException e) {
          e.printStackTrace();
          callback.onResponse(LynxResourceResponse.onFailed(e));
        }
      }

      @Override
      public void onFailure(Call<ResponseBody> call, Throwable throwable) {
        callback.onResponse(LynxResourceResponse.onFailed(throwable));
      }
    });
  }

  @Override
  public void fetchResourcePath(
      LynxResourceRequest request, LynxResourceCallback<String> callback) {
    callback.onResponse(
        LynxResourceResponse.onFailed(new Throwable("fetchResourcePath not supported.")));
  }

  @Override
  public void fetchStream(LynxResourceRequest request, StreamDelegate delegate) {
    delegate.onError("fetchStream not supported.");
  }

  @Override
  public void cancel(LynxResourceRequest request) {}
}

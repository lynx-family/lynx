// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import java.io.IOException;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.Response;
import retrofit2.Retrofit;

public class DemoTemplateResourceFetcher extends LynxTemplateResourceFetcher {
  private final Context mApplicationContext;

  public DemoTemplateResourceFetcher(@NonNull Context context) {
    mApplicationContext = context.getApplicationContext();
  }

  private void requestResource(
      LynxResourceRequest request, retrofit2.Callback<ResponseBody> callback) {
    Retrofit retrofit = new Retrofit.Builder()
                            .baseUrl("https://example.com/")
                            .callbackExecutor(LynxThreadPool.getBriefIOExecutor())
                            .build();

    TemplateApi templateApi = retrofit.create(TemplateApi.class);
    Call<ResponseBody> call = templateApi.getTemplate(request.getUrl());
    call.enqueue(callback);
  }

  @Override
  public void fetchTemplate(
      LynxResourceRequest request, LynxResourceCallback<TemplateProviderResult> callback) {
    if (request == null) {
      callback.onResponse(LynxResourceResponse.onFailed(new Throwable("request is null!")));
      return;
    }

    String url = request.getUrl();
    if (LynxViewShellActivity.isAssetFilename(url)) {
      url = LynxViewShellActivity.getAssetFilename(url);
      readBundleFromAssets(url, callback);
      return;
    }

    requestResource(request, new retrofit2.Callback<ResponseBody>() {
      @Override
      public void onResponse(Call<ResponseBody> call, Response<ResponseBody> response) {
        if (response.body() != null) {
          try {
            TemplateProviderResult result =
                TemplateProviderResult.fromBinary(response.body().bytes());
            callback.onResponse(LynxResourceResponse.onSuccess(result));
          } catch (IOException e) {
            e.printStackTrace();
            callback.onResponse(LynxResourceResponse.onFailed(e));
          }
        }
      }

      @Override
      public void onFailure(Call<ResponseBody> call, Throwable throwable) {
        callback.onResponse(LynxResourceResponse.onFailed(throwable));
      }
    });
  }

  private void readBundleFromAssets(
      String url, LynxResourceCallback<TemplateProviderResult> callback) {
    LynxThreadPool.getBriefIOExecutor().execute(new Runnable() {
      @Override
      public void run() {
        if (mApplicationContext == null) {
          callback.onResponse(LynxResourceResponse.onFailed(new Throwable("Context is null.")));
          return;
        }
        byte[] data = LynxViewShellActivity.readFileFromAssets(mApplicationContext, url);
        if (data != null) {
          TemplateProviderResult result = TemplateProviderResult.fromBinary(data);
          callback.onResponse(LynxResourceResponse.onSuccess(result));
        } else {
          callback.onResponse(LynxResourceResponse.onFailed(new Throwable("Unable to read file.")));
        }
      }
    });
  }

  @Override
  public void fetchSSRData(LynxResourceRequest request, LynxResourceCallback<byte[]> callback) {
    if (request == null) {
      callback.onResponse(LynxResourceResponse.onFailed(new Throwable("request is null!")));
      return;
    }

    requestResource(request, new retrofit2.Callback<ResponseBody>() {
      @Override
      public void onResponse(Call<ResponseBody> call, Response<ResponseBody> response) {
        try {
          if (response.body() != null) {
            callback.onResponse(LynxResourceResponse.onSuccess(response.body().bytes()));
          } else {
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
}

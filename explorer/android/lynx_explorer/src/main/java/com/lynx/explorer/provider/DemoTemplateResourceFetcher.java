// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import android.content.Context;
import android.net.Uri;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.Response;
import retrofit2.Retrofit;

public class DemoTemplateResourceFetcher extends LynxTemplateResourceFetcher {
  private static final int HTTP_TIMEOUT_SECONDS = 5;
  private final Context mApplicationContext;

  public DemoTemplateResourceFetcher(@NonNull Context context) {
    mApplicationContext = context.getApplicationContext();
  }

  private boolean shouldSendAsyncRequest(String url) {
    if (TextUtils.isEmpty(url)) {
      return true;
    }
    Uri uri = Uri.parse(url);
    if (!uri.isHierarchical()) {
      return true;
    }
    String syncParam = uri.getQueryParameter("sync");
    if (syncParam != null) {
      return !syncParam.equals("true");
    }
    return true;
  }

  private void requestResource(
      LynxResourceRequest request, retrofit2.Callback<ResponseBody> callback) {
    createRequest(request, false).enqueue(callback);
  }

  private Call<ResponseBody> createRequest(LynxResourceRequest request, boolean sync) {
    Retrofit retrofit =
        new Retrofit.Builder()
            .baseUrl("https://example.com/")
            .callbackExecutor(sync ? Runnable::run : LynxThreadPool.getBriefIOExecutor())
            .build();

    TemplateApi templateApi = retrofit.create(TemplateApi.class);
    return templateApi.getTemplate(request.getUrl());
  }

  @Override
  public void fetchTemplate(
      LynxResourceRequest request, LynxResourceCallback<TemplateProviderResult> callback) {
    if (request == null) {
      callback.onResponse(LynxResourceResponse.onFailed(new Throwable("request is null!")));
      return;
    }

    String url = request.getUrl();
    boolean isAsset = LynxViewShellActivity.isAssetFilename(url);
    String resourceUrl = isAsset ? LynxViewShellActivity.getAssetFilename(url) : url;
    boolean sync = !shouldSendAsyncRequest(resourceUrl);
    if (isAsset) {
      Uri uri = Uri.parse(resourceUrl);
      url = uri.buildUpon().clearQuery().fragment(null).build().toString();
      readBundleFromAssets(url, callback, sync);
      return;
    }

    if (sync) {
      CountDownLatch latch = new CountDownLatch(1);
      AtomicReference<LynxResourceResponse<TemplateProviderResult>> response =
          new AtomicReference<>();
      Call<ResponseBody> call = fetchNetworkTemplate(request, result -> {
        response.set(result);
        latch.countDown();
      }, true);
      LynxResourceResponse<TemplateProviderResult> result;
      try {
        if (latch.await(HTTP_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
          result = response.get();
        } else {
          call.cancel();
          result =
              LynxResourceResponse.onFailed(new TimeoutException("Template request timed out"));
        }
      } catch (InterruptedException e) {
        call.cancel();
        Thread.currentThread().interrupt();
        result = LynxResourceResponse.onFailed(e);
      }
      callback.onResponse(result);
      return;
    }
    fetchNetworkTemplate(request, callback, false);
  }

  private Call<ResponseBody> fetchNetworkTemplate(LynxResourceRequest request,
      LynxResourceCallback<TemplateProviderResult> callback, boolean sync) {
    // Synchronous callers wait for this result; deliver their network result directly on the
    // OkHttp worker so a blocked caller cannot exhaust the callback executor.
    Call<ResponseBody> call = createRequest(request, sync);
    call.enqueue(new retrofit2.Callback<ResponseBody>() {
      @Override
      public void onResponse(Call<ResponseBody> call, Response<ResponseBody> response) {
        if (!response.isSuccessful() || response.body() == null) {
          if (response.errorBody() != null) {
            response.errorBody().close();
          }
          callback.onResponse(LynxResourceResponse.onFailed(
              new IOException("HTTP request failed: " + response.code())));
          return;
        }
        try (ResponseBody body = response.body()) {
          TemplateProviderResult result = TemplateProviderResult.fromBinary(body.bytes());
          callback.onResponse(LynxResourceResponse.onSuccess(result));
        } catch (IOException e) {
          callback.onResponse(LynxResourceResponse.onFailed(e));
        }
      }

      @Override
      public void onFailure(Call<ResponseBody> call, Throwable throwable) {
        callback.onResponse(LynxResourceResponse.onFailed(throwable));
      }
    });
    return call;
  }

  private void readBundleFromAssets(
      String url, LynxResourceCallback<TemplateProviderResult> callback, boolean sync) {
    Runnable readBundle = new Runnable() {
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
    };
    if (sync) {
      readBundle.run();
    } else {
      LynxThreadPool.getBriefIOExecutor().execute(readBundle);
    }
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

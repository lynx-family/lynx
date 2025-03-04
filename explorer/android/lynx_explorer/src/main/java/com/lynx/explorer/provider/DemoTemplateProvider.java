// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import com.lynx.tasm.provider.AbsTemplateProvider;
import java.io.IOException;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.Response;
import retrofit2.Retrofit;

public class DemoTemplateProvider extends AbsTemplateProvider {
  @Override
  public void loadTemplate(String url, final Callback callback) {
    Retrofit retrofit = new Retrofit.Builder().baseUrl("https://example.com/").build();

    TemplateApi templateApi = retrofit.create(TemplateApi.class);

    Call<ResponseBody> call = templateApi.getTemplate(url);

    call.enqueue(new retrofit2.Callback<ResponseBody>() {
      @Override
      public void onResponse(Call<ResponseBody> call, Response<ResponseBody> response) {
        try {
          if (response.body() != null) {
            callback.onSuccess(response.body().bytes());
          } else {
          }
        } catch (IOException e) {
          e.printStackTrace();
          callback.onFailed(e.toString());
        }
      }

      @Override
      public void onFailure(Call<ResponseBody> call, Throwable throwable) {
        callback.onFailed(throwable.getMessage());
      }
    });
  }
}

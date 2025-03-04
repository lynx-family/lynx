// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import java.util.Map;
import okhttp3.RequestBody;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.FieldMap;
import retrofit2.http.GET;
import retrofit2.http.HeaderMap;
import retrofit2.http.POST;
import retrofit2.http.Url;

public interface TemplateApi {
  @GET Call<ResponseBody> getTemplate(@Url String u);

  @GET Call<ResponseBody> get(@Url String url, @HeaderMap Map<String, String> headers);

  @POST
  Call<ResponseBody> postForm(@Url String url, @FieldMap Map<String, String> fields,
      @HeaderMap Map<String, String> headers);

  @POST
  Call<ResponseBody> postData(
      @Url String url, @Body RequestBody data, @HeaderMap Map<String, String> headers);
}

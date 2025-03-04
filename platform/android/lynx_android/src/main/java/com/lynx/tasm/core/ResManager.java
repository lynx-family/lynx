// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core;

import android.content.Context;
import android.text.TextUtils;
import android.util.LruCache;
import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.provider.LynxResCallback;
import com.lynx.tasm.provider.LynxResRequest;
import com.lynx.tasm.provider.LynxResResponse;
import com.lynx.tasm.provider.ResProvider;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.CountDownLatch;

public class ResManager {
  private static final String TAG = "lynx_ResManager";

  /**
   * http scheme for URIs
   */
  public static final String HTTP_SCHEME = "http://";
  public static final String HTTPS_SCHEME = "https://";

  /**
   * Asset scheme for URIs
   */
  public static final String LOCAL_ASSET_SCHEME = "asset:///";

  /**
   * Resource scheme for URIs  example:res:///drawable/icon
   */
  public static final String LOCAL_RESOURCE_SCHEME = "res:///";

  /**
   * File scheme for URIs
   */
  public static final String FILE_SCHEME = "file://";

  private static ResManager sInstance;

  private static final int MAX_ID_CACHE_SIZE = 100;
  private LruCache<String, Integer> mIdCache;

  public static ResManager inst() {
    if (null == sInstance) {
      synchronized (ResManager.class) {
        if (null == sInstance) {
          sInstance = new ResManager();
        }
      }
    }
    return sInstance;
  }

  private ResManager() {
    mIdCache = new LruCache<>(MAX_ID_CACHE_SIZE);
  }

  @Nullable
  @WorkerThread
  public LynxResResponse requestResourceSync(@NonNull LynxResRequest request) {
    final CountDownLatch latch = new CountDownLatch(1);
    final LynxResResponse[] ret = new LynxResResponse[1];
    try {
      requestResource(request, new LynxResCallback() {
        @Override
        public void onSuccess(@NonNull LynxResResponse response) {
          ret[0] = response;
          latch.countDown();
        }

        @Override
        public void onFailed(@NonNull LynxResResponse response) {
          ret[0] = response;
          latch.countDown();
        }
      });
    } catch (Throwable e) {
      // ignore
      latch.countDown();
    }

    try {
      latch.await();
    } catch (InterruptedException e) {
      LLog.e(TAG, "sync await failed:" + e.toString());
    }
    return ret[0];
  }

  /**
   * async function
   *
   * @param request
   * @param callback
   */
  @AnyThread
  public void requestResource(
      final @NonNull LynxResRequest request, @NonNull final LynxResCallback callback) {
    final LynxResResponse response = new LynxResResponse();
    final String url = request.getUrl();
    if (TextUtils.isEmpty(url)) {
      response.setReasonPhrase("url is empty!");
      callback.onFailed(response);
      LLog.w(TAG, "url:" + url + " is empty!");
      return;
    }
    LynxThreadPool.getBriefIOExecutor().execute(new Runnable() {
      @Override
      public void run() {
        if ((url.startsWith(HTTP_SCHEME) || url.startsWith(HTTPS_SCHEME))
            && url.length() > HTTPS_SCHEME.length()) {
          doFetch(request, callback);
        } else if (url.startsWith(LOCAL_ASSET_SCHEME)
            && url.length() > LOCAL_ASSET_SCHEME.length()) {
          doFetchAssets(url, callback);
        } else if (url.startsWith(LOCAL_RESOURCE_SCHEME)
            && url.length() > LOCAL_RESOURCE_SCHEME.length()) {
          doFetchRes(url, callback);
        } else if (url.startsWith(FILE_SCHEME) && url.length() > FILE_SCHEME.length()) {
          doFetchFile(url, callback);
        } else {
          LLog.DTHROW(new RuntimeException("illegal url:" + url));
          response.setReasonPhrase("url is illegal:" + url);
          callback.onFailed(response);
        }
      }
    });
  }

  private void doFetch(
      final @NonNull LynxResRequest request, @NonNull final LynxResCallback callback) {
    ResProvider provider = LynxEnv.inst().getResProvider();
    LLog.DCHECK(null != provider);
    if (null == provider) {
      LynxResResponse response = new LynxResResponse();
      response.setReasonPhrase("don't have ResProvider.Can't Get Resource.");
      callback.onFailed(response);
      return;
    }
    provider.request(request, callback);
  }

  private void doFetchAssets(@NonNull final String url, @NonNull final LynxResCallback callback) {
    InputStream is = null;
    LynxResResponse response = new LynxResResponse();
    try {
      is = LynxEnv.inst().getAppContext().getAssets().open(
          url.substring(LOCAL_ASSET_SCHEME.length()));
      StringBuilder sb = new StringBuilder(is.available());
      byte[] buffer = new byte[1024];
      int numRead = 0;
      while ((numRead = is.read(buffer)) != -1) {
        sb.append(new String(buffer, 0, numRead));
      }
      InputStream stream = new ByteArrayInputStream(sb.toString().getBytes());
      response.setInputStream(stream);
      callback.onSuccess(response);
      stream.close();
    } catch (IOException e) {
      response.setReasonPhrase(e.toString());
      callback.onFailed(response);
    } finally {
      if (null != is) {
        try {
          is.close();
        } catch (IOException e) {
          // ignore
        }
      }
    }
  }

  private void doFetchRes(@NonNull final String url, @NonNull final LynxResCallback callback) {
    Integer resId =
        findResId(LynxEnv.inst().getAppContext(), url.substring(LOCAL_RESOURCE_SCHEME.length()));
    LynxResResponse response = new LynxResResponse();
    if (null != resId) {
      InputStream stream = new ByteArrayInputStream(new byte[] {resId.byteValue()});
      response.setInputStream(stream);
      callback.onSuccess(response);
      try {
        stream.close();
      } catch (IOException e) {
        // ignore
      }
    } else {
      response.setReasonPhrase("resource not found!");
      callback.onFailed(response);
    }
  }

  private void doFetchFile(@NonNull final String url, @NonNull final LynxResCallback callback) {
    String path = url.substring(FILE_SCHEME.length());
    final File file;
    if (path.startsWith("/")) {
      file = new File(path);
    } else {
      file = new File(LynxEnv.inst().getAppContext().getFilesDir(), path);
    }
    LynxResResponse response = new LynxResResponse();
    try {
      InputStream stream = new FileInputStream(file);
      StringBuilder sb = new StringBuilder(stream.available());
      byte[] buffer = new byte[1024];
      int numRead = 0;
      while ((numRead = stream.read(buffer)) != -1) {
        sb.append(new String(buffer, 0, numRead));
      }
      InputStream Sstream = new ByteArrayInputStream(sb.toString().getBytes());
      response.setInputStream(Sstream);
      callback.onSuccess(response);
      stream.close();
    } catch (FileNotFoundException e) {
      response.setReasonPhrase("file not found!");
      callback.onFailed(response);
    } catch (IOException e) {
      response.setReasonPhrase("IO failed");
      callback.onFailed(response);
    }
  }

  @Nullable
  public Integer findResId(Context context, @Nullable String name) {
    if (name == null || name.isEmpty()) {
      return null;
    }
    // name could be a resource id.
    try {
      return Integer.parseInt(name);
    } catch (NumberFormatException e) {
      // Do nothing.
    }

    name = name.toLowerCase().replace("-", "_");
    Integer id = mIdCache.get(name);
    if (id != null) {
      return id;
    }
    int i = name.indexOf(".");
    String defType = null;
    if (i > 0 && i < name.length()) {
      defType = name.substring(0, i);
    }
    if (TextUtils.isEmpty(defType)) {
      return null;
    }

    synchronized (this) {
      id = context.getResources().getIdentifier(name, defType, context.getPackageName());
      if (id > 0) {
        mIdCache.put(name, id);
      }
    }
    return id;
  }
}

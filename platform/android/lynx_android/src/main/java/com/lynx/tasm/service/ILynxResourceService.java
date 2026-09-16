// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.content.Context;
import android.graphics.Typeface;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.io.Closeable;

public interface ILynxResourceService extends IServiceProvider {
  /**
   *  code for exception
   */
  int RESULT_EXCEPTION = -1;

  /**
   * code for isLocalResource
   */
  int RESULT_IS_NOT_LOCAL_RESOURCE = 0;
  int RESULT_IS_LOCAL_RESOURCE = 1;

  /**
   * code of empty url for fetchResource
   */
  int RESULT_EMPTY_URL = 2;
  /**
   * code of invalid url for fetchResource
   */
  int RESULT_INVALID_URL = 3;

  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxResourceService.class;
  }

  /**
   * @return true if the service is ready, false otherwise.
   */
  boolean isReady();

  /**
   * get resource local status for url
   * @param url
   * @return code for isLocalResource or exception
   */
  int isLocalResource(@Nullable String url);

  /**
   * preload media callback
   */
  interface PreloadMediaCallback {
    void onComplete(int code, String msg);
  }

  /**
   * preload media with callback
   * @param url
   * @param preloadKey key of preload media
   * @param videoID video id
   * @param size size of preload media
   * @param callback callback
   */
  void preloadMedia(String url, String preloadKey, @Nullable String videoID, long size,
      @Nullable PreloadMediaCallback callback);

  /**
   * cancel preload media
   * @param preloadKey key of preload media
   * @param videoID video id
   */
  void cancelPreloadMedia(String preloadKey, @Nullable String videoID);

  /**
   * add resource loader for target templateUrl
   * @param loader
   * @param templateUrl
   */
  void addResourceLoader(@NonNull Object loader, @NonNull String templateUrl);

  /**
   * fetch resource async
   * @param url
   * @param lynxResourceServiceRequestParams
   * @param callback
   * @return ILynxResourceServiceRequestOperation
   */
  @Nullable
  ILynxResourceServiceRequestOperation fetchResourceAsync(@Nullable String url,
      @NonNull LynxResourceServiceRequestParams lynxResourceServiceRequestParams,
      @NonNull final LynxResourceServiceCallback callback);

  /**
   * fetch resource sync
   * @param url
   * @param lynxResourceServiceRequestParams
   * @return ILynxResourceServiceResponse
   */
  @Nullable
  ILynxResourceServiceResponse fetchResourceSync(@Nullable String url,
      @NonNull LynxResourceServiceRequestParams lynxResourceServiceRequestParams);

  /**
   * Create a typeface from the specified resource URL or file path.
   * @param url resource URL or file path
   * @return typeface if creation succeeds, otherwise null
   */
  @Nullable Typeface createTypeFace(@NonNull String url);

  /**
   * Creates a platform media data source for a URI that requires custom resource handling.
   *
   * <p>The returned object is an {@code android.media.MediaDataSource} on Android M and above. It
   * is declared as {@link Closeable} so loading this interface remains safe on earlier Android
   * versions where {@code android.media.MediaDataSource} does not exist. An implementation should
   * return {@code null} when it does not handle the URI, cannot create the data source, or the
   * platform does not support it. The caller will then fall back to the platform's default URI
   * handling. Ownership of a non-null result is transferred to the caller.
   *
   * @param context Android context used to resolve the URI
   * @param uri URI of the media resource
   * @return a platform media data source when the URI is handled successfully, otherwise {@code
   *     null}
   */
  @Nullable
  default Closeable createMediaDataSource(@NonNull Context context, @NonNull Uri uri) {
    return null;
  }
}

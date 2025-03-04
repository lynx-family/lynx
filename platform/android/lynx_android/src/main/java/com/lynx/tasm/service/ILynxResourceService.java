// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

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
   * preload media
   * @param url
   * @param preloadKey key of preload media
   * @param videoID video id
   */
  void preloadMedia(String url, String preloadKey, @Nullable String videoID, long size);

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
}

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceRequest } from './LynxResourceRequest';
import { LynxResourceProvider } from './LynxResourceProvider';
import { AsyncCallback } from '@ohos.base';

export enum LynxOptionalBool {
  TRUE,
  FALSE,
  UNDEFINED
}

/**
 * `LynxMediaResourceFetcher` is defined inside LynxEngine and
 * injected from outside to implement the path redirection capability
 * of `Image` and other third-party resources.
 * @apidoc
 */
export abstract class LynxMediaResourceFetcher implements LynxResourceProvider {
  /**
   * `LynxEngine` redirects the image path by calling this method internally,
   * and the return result is required to be of `String` type
   *
   * @apidoc
   * @param request Request to be redirected.
   * @return The target url.
   */
  abstract shouldRedirectUrl(request: LynxResourceRequest): string;

  /**
   * `LynxEngine` internally calls this method to determine whether the
   * resource path exists on disk.
   *
   * @apidoc
   * @param url Input path.
   * @returns TRUE if is a local path, FALSE if not a local path, UNDEFINED if not sure.
   */
  isLocalResource(url: string): LynxOptionalBool {
    return LynxOptionalBool.UNDEFINED
  };

  /**
   * `LynxEngine` redirects the image URL asynchronously by calling this method internally.
   * The redirected URL, or an array of URLs including fallback URLs, should be returned via the callback.
   *
   * @apidoc
   * @param request Request to be redirected.
   * @param callback The callback used to return the target URL string or an array of URL strings (primary URL and fallback URLs).
   */
  shouldRedirectUrlAsync(request: LynxResourceRequest, callback: AsyncCallback<string | Array<string>>) {
    callback(undefined, request.url);
  }

  // TODO(nihao.royal): support fetchImage api.
}

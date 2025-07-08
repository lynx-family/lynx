// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceRequest } from './LynxResourceRequest';
import { LynxResourceProvider } from './LynxResourceProvider';

export enum LynxOptionalBool {
  TRUE,
  FALSE,
  UNDEFINED
}

export abstract class LynxMediaResourceFetcher implements LynxResourceProvider {
  /**
   * redirect url path
   *
   * @param request
   */
  abstract shouldRedirectUrl(request: LynxResourceRequest): string;

  /**
   * Quick check for a local path
   *
   * @param url input path
   * @returns
   *  TRUE if is a local path;
   *  FALSE if not a local path;
   *  UNDEFINED if not sure;
   */
  isLocalResource(url: string): LynxOptionalBool {
    return LynxOptionalBool.UNDEFINED
  };

  // TODO(nihao.royal): support fetchImage api.
}
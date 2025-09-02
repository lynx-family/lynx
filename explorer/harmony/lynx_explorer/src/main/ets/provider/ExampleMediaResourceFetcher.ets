// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxMediaResourceFetcher, LynxOptionalBool, LynxResourceRequest } from '@lynx/lynx';

export class ExampleMediaResourceFetcher extends LynxMediaResourceFetcher {
  shouldRedirectUrl(request: LynxResourceRequest): string {
    if (request.url.startsWith('local://')) {
      return 'resource://rawfile/' + request.url.slice(8);
    }
    return request.url;
  }

  isLocalResource(url: string): LynxOptionalBool {
    return LynxOptionalBool.UNDEFINED;
  }
}

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { LynxResourceRequest } from "../../Index";
import { AsyncCallback } from "@kit.BasicServicesKit";

export interface LynxImageConfig {
  /**
   * Whether to enable image load callback.
   */
  enableImageLoadCallback?: boolean;

  enableTransformUrl?: boolean;

  transformUrl?: (request: LynxResourceRequest, callback: AsyncCallback<string>) => void;
}

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxAbstractMapView } from './LynxAbstractMapView';

export enum LynxMapType {
  UNKNOWN = -1,
  ALI_2D = 0,
  ALI = 1,
  GOOGLE = 2,
  GOOGLE_STATIC = 3,
  BAIDU = 4
}

export abstract class LynxMapProvider {
  abstract createMapView(): LynxAbstractMapView;
}

// Register before creating the page. The provider interprets map-type during
// initialization and supplies an independent view for each map.
export class LynxXElementMapService {
  static provider?: LynxMapProvider;

  static registerProvider(provider: LynxMapProvider): void {
    LynxXElementMapService.provider = provider;
  }
}

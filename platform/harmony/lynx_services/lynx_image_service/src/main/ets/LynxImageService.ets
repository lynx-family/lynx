// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ILynxImageService } from '@lynx/lynx';
import lynx_image_service from 'liblynx_image_service.so';

export class LynxImageService implements ILynxImageService {
  initNativeService(resourceManager: Object): number[] {
    return lynx_image_service.nativeInitLynxImageService(resourceManager);
  }
  static instance = new LynxImageService();

  private constructor() {
  }
}
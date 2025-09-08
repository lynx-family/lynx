// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { ILynxBaseTraceService } from '@lynx/lynx_base'

export class LynxBaseTraceService implements ILynxBaseTraceService{
  static instance = new LynxBaseTraceService();
  
  private LynxBaseTraceService() {
  }
  
  getDefaultTraceFunction(): number {
    return lynx.getBaseTraceBackend();
  }
}

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxModule } from '../jsbridge/LynxModule';

export class LynxSetModule extends LynxModule {
  // Do nothing to align with Android
  public switchKeyBoardDetect(arg: boolean) {
    return;
  }
}

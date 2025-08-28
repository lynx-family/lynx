// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const TAG = 'LynxBaseEnv';
class LynxBaseEnvironment {
  private initialized = false;
  public initialize() {
    if (this.initialized) {
      return;
    }
    // do something.
    this.initialized = true;
  }
}
export const LynxBaseEnv = new LynxBaseEnvironment();
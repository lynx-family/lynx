// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxGroup } from './LynxGroup';

export class LynxBackgroundRuntimeOptions {
  public enableBytecode: boolean = false;
  public bytecodeSourceUrl: string = '';
  public lynxGroup?: LynxGroup;

  constructor(options?: LynxBackgroundRuntimeOptions) {
    if (options) {
      this.lynxGroup = LynxGroup.copy(options.lynxGroup);
      this.enableBytecode = options.enableBytecode;
      this.bytecodeSourceUrl = options.bytecodeSourceUrl;
    }
  }

  useQuickJSEngine(): boolean {
    return this.lynxGroup ? (!this.lynxGroup.enableDynamicV8 && !this.lynxGroup.enableHarmonyJSVM) : true;
  }

  getGroupID(): string {
    return this.lynxGroup ? this.lynxGroup.getGroupID() : LynxGroup.SINGLE_GROUP;
  }

  getPreloadJSPath(): string[] {
    return this.lynxGroup ? this.lynxGroup.preloadJSPaths : [];
  }

  enableJSGroupThread(): boolean {
    return this.lynxGroup ? this.lynxGroup.enableJSGroupThread : false;
  }
}

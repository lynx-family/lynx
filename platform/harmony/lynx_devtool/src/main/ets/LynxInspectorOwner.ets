// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxBaseInspectorOwner, LynxView } from '@lynx/lynx';
import { DebugRouter } from './DebugRouter';

import lynxdevtool from 'liblynxdevtool.so';

export class LynxInspectorOwner implements LynxBaseInspectorOwner {
  private nativeOwner: lynxdevtool.InspectorOwnerHarmony;

  constructor(embedderProxy: number[]) {
    this.nativeOwner = new lynxdevtool.InspectorOwnerHarmony(embedderProxy);
  }

  public onLoaded(lynxView: LynxView) {
    let sessionId: number = this.nativeOwner.getSessionId();
    DebugRouter.setSessionIdOfView(lynxView, sessionId);
  }

  public destroy(lynxView: LynxView): void {
    DebugRouter.removeView(lynxView);
    this.nativeOwner.destroy();
  }
}
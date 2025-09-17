// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxView } from '../tasm/LynxView';
import { LynxInspectorConsoleDelegate } from './LynxInspectorConsoleDelegate'

type CDPResultCallback = (result: string) => void;

export interface LynxBaseInspectorOwner {
  onLoaded(lynxView: LynxView): void;

  destroy(lynxView: LynxView): void;
  
  setLynxInspectorConsoleDelegate(delegate: LynxInspectorConsoleDelegate): void;
  
  getConsoleObject(objectId: string, needStringify: boolean, callback: (detail: string) => void): void;
}


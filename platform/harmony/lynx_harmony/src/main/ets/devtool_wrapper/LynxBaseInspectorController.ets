// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxView } from '../tasm/LynxView';

export interface LynxBaseInspectorController {
  attachProxy(embedderProxy: number[]): void;

  onLoaded(lynxView: LynxView): void;

  destroy(lynxView: LynxView): void;
}


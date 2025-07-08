// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxExtraTiming } from './LynxExtraTiming';


export interface ITimingCollector {
  setTiming(timestamp: number, timingKey: string, pipelineID?: string): void;

  markPaintEndTimingIfNeeded(): void;

  setExtraTiming(extraTiming: LynxExtraTiming): void;
}

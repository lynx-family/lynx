// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export { getBridgeReadyPromise } from './delay';
import { DelayBridgeImpl } from './delay';
import { ILogBoxBridge } from '@/jsbridge/interface';
import { ImmBridgeImpl } from './immediate';

function isBridgeExists(): boolean {
  return !!(window as { logbox?: Object }).logbox;
}

export function getBridge(): ILogBoxBridge {
  if (isBridgeExists()) {
    return ImmBridgeImpl;
  } else {
    return DelayBridgeImpl;
  }
}

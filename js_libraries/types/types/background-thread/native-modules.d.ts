// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { AnyObject } from '../common';
import { NODE_REF_INVOKE_ERROR_CODE } from './nodes-ref';

export interface NativeModules {
  LynxUIMethodModule?: {
    invokeUIMethod?(componentId: string, ancestors: string[], method: string, params: AnyObject, callback: (res: { code: NODE_REF_INVOKE_ERROR_CODE }) => void): void;
  };
  NetworkingModule?: any;
  LynxTestModule?: any;

  [key: string]: any;

  bridge: {
    call: (name: string, params: Record<string, unknown>, cb: (...args: unknown[]) => void) => void;
    on: (name: string, cb: (...args: unknown[]) => void) => void;
  };
}

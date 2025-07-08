// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxError } from '../tasm/LynxError';

export type OnErrorCallback = (msg: string) => void;

export type GetJSSourceCallback = () => Record<string, string>;

export interface ILogBox {
  reportError(err: LynxError): void;

  onLoaded(url: string): void;
}

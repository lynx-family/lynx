// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxContext } from '../../tasm/LynxContext';
import { IServiceProvider } from './IServiceProvider';

export enum LynxTasmType {
  TYPE_TEMPLATE = 0,
  TYPE_DYNAMIC_COMPONENT = 1
}

export interface SecurityResult {
  verified: boolean;
  errMsg?: string;
}

export interface ILynxSecurityService extends IServiceProvider {
  /**
   * use specified verify logic to check the template consistency.
   *
   * @param template: the input tasm binary.
   * @param url: the input tasm identify.
   *
   * @return result of the verification.
   */
  verifyTASM(template: ArrayBuffer, url: string | undefined, type: LynxTasmType,
    lynxContext?: LynxContext): SecurityResult;
}
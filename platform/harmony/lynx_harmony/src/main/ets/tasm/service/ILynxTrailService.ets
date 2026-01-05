// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';
import { LynxView } from '../LynxView';
import { LynxServiceCenter } from '../../Index';
import { IServiceExtension } from './IServiceExtension';

export interface ILynxTrailService extends IServiceProvider {
}

export const LynxTrailServiceType = LynxServiceCenter.defineServiceType<ILynxTrailService>();

export interface ILynxTrailServiceExtension extends IServiceExtension {

  /**
   * Parse LynxView
   * @param LynxView
   */
  parseLynxView(view: LynxView): void;
}

export const LynxTrailServiceExtensionType = LynxServiceCenter.defineServiceExtensionType<ILynxTrailServiceExtension>();

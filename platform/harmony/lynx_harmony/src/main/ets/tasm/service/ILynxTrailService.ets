// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';
import common from '@ohos.app.ability.common';
import { LynxView } from '../LynxView';

export interface ILynxTrailService extends IServiceProvider {
  /**
   * initialize LynxTrailService
   * @param context
   */
  initialize(context: common.ApplicationContext): void

  /**
   * Parse LynxView
   * @param LynxView
   */
  parseLynxView(view: LynxView): void;
}
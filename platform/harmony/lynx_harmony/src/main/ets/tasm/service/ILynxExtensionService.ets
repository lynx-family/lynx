// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';
import { LynxContext } from '../LynxContext';

export interface ILynxExtensionService extends IServiceProvider {
  onLynxViewSetup(context: LynxContext, uiDelegate: number[], isHostRenderer: boolean): void;

  onLynxViewDestroy(context: LynxContext): void;
}
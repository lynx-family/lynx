// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxContext } from '../tasm/LynxContext';
import { lang } from '@kit.ArkTS';

type ISendable = lang.ISendable;

export class LynxModule {
  protected context: LynxContext;
  protected param?: Object;

  constructor(context: LynxContext, param?: Object) {
    this.context = context;
    this.param = param;
  }
}

export interface ModuleClassWrapper {
  moduleClass: Function;
  param?: Object;
}

@Sendable
export class SendableLynxModule {
  protected param?: ISendable;

  constructor(param?: ISendable) {
    this.param = param;
  }
}

@Sendable
export class SendableModuleClassWrapper {
  moduleClass?: ISendable;
  param?: ISendable;

  constructor(moduleClass: ISendable, params?: ISendable) {
    this.moduleClass = moduleClass;
    this.param = params;
  }
}

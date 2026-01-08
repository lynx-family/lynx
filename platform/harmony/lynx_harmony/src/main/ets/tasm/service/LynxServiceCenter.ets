// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';

const TAG = 'LynxServiceCenter';

export enum LynxServiceType {
  Trail,
  EventReporter,
  Http,
  Security,
  Extension,
  DevTool,
}

class LynxServiceCenterImpl {
  private serviceMap: Map<LynxServiceType, IServiceProvider> = new Map();

  constructor() {
  }

  getService<T>(serviceType: LynxServiceType): T | undefined {
    return this.serviceMap.get(serviceType) as T;
  }

  registerService(serviceType: LynxServiceType, instance: IServiceProvider) {
    this.serviceMap.set(serviceType, instance);
  }

  unregisterService(serviceType: LynxServiceType,) {
    this.serviceMap.delete(serviceType);
  }
}

export const LynxServiceCenter = new LynxServiceCenterImpl()
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IBaseServiceProvider } from './IBaseServiceProvider';

const TAG = 'LynxBaseServiceCenter';

export enum LynxBaseServiceType {
  Log,
  Trace,
}

class LynxBaseServiceCenterImpl {
  private serviceMap: Map<LynxBaseServiceType, IBaseServiceProvider> = new Map();

  constructor() {
  }

  getService<T>(serviceType: LynxBaseServiceType): T | undefined {
    return this.serviceMap.get(serviceType) as T;
  }

  registerService(serviceType: LynxBaseServiceType, instance: IBaseServiceProvider) {
    this.serviceMap.set(serviceType, instance);
  }

  unregisterService(serviceType: LynxBaseServiceType,) {
    this.serviceMap.delete(serviceType);
  }
}

export const LynxBaseServiceCenter = new LynxBaseServiceCenterImpl()
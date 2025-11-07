// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceExtension, ServiceExtensionType } from './IServiceExtension';
import { IServiceProvider, ServiceType } from './IServiceProvider';
import common from '@ohos.app.ability.common';

const TAG = 'LynxServiceCenter';

class LynxServiceCenterImpl {
  private serviceMap: Map<ServiceType<IServiceProvider>, IServiceProvider> = new Map();
  private initialized = false;
  private context!: common.ApplicationContext;

  constructor() {
  }

  /**
   * Triggers all services' IServiceProvider.onInitialize
   * <p>
   * This method can be invoked multiple times, while each service's
   * IServiceProvider.onInitialize will be guaranteed by LynxServiceCenter
   * to be invoked only once per service. Therefore, the frameworks like
   * lynx can safely call this method as needed.
   */
  initialize(context: common.ApplicationContext) {
    if (this.initialized) {
      return;
    }
    this.initialized = true;
    this.context = context;
    for (const impl of this.serviceMap.values()) {
      impl.onInitialize?.(context);
    }
  }

  defineServiceType<T extends IServiceProvider>(): ServiceType<T> {
    return new ServiceType();
  }

  defineServiceExtensionType<T extends IServiceExtension>(): ServiceExtensionType<T> {
    return new ServiceExtensionType();
  }

  getService<T extends IServiceProvider>(serviceType: ServiceType<T>): T | undefined {
    return this.serviceMap.get(serviceType) as T;
  }

  registerService<T extends IServiceProvider>(serviceType: ServiceType<T>, instance: T) {
    if (this.initialized) {
      instance.onInitialize?.(this.context);
    }
    this.serviceMap.set(serviceType, instance);
  }

  unregisterService<T extends IServiceProvider>(serviceType: ServiceType<T>) {
    this.serviceMap.delete(serviceType);
  }

  /**
   * Get service and try casting to service extension.
   *
   * @param serviceType
   * @param extensionType
   * @returns
   */
  getServiceAsExtension<ST extends IServiceProvider, ET extends IServiceExtension>(
    serviceType: ServiceType<ST>,
    extensionType: ServiceExtensionType<ET>
  ): ET | undefined {
    const service = this.getService(serviceType);
    if (service?.extensions?.has(extensionType)) {
      return service as object as ET;
    }
    return undefined
  }
}

export const LynxServiceCenter = new LynxServiceCenterImpl()

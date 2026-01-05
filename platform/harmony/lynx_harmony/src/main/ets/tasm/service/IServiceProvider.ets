// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { ServiceExtensionType } from "./IServiceExtension";
import common from "@ohos.app.ability.common";

export interface IServiceProvider {
  extensions?: Set<ServiceExtensionType>;
  
  onInitialize?: (context: common.ApplicationContext) => void;
}

export class ServiceType<T extends IServiceProvider> {
}

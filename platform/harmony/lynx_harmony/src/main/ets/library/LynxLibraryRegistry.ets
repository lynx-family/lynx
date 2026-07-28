// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ModuleClassWrapper } from '../jsbridge/LynxModule';
import { LynxModuleManager } from '../jsbridge/LynxModuleManager';
import { Behavior, BUILTIN_BEHAVIORS } from '../tasm/behavior/Behavior';
import { IServiceProvider } from '../tasm/service/IServiceProvider';
import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';

export type LynxLibraryInitializer = () => void;

export interface LynxLibraryProvider {
  register(registry: LynxLibraryRegistry): void;
}

export interface LynxLibraryProviderEntry {
  packageName: string;
  provider: LynxLibraryProvider;
}

const GLOBAL_MODULES: Map<string, ModuleClassWrapper> = new Map();
const REGISTERED_PACKAGES: Set<string> = new Set();
const BEHAVIOR_OWNERS: Map<string, string> = new Map();
const MODULE_OWNERS: Map<string, string> = new Map();
const SERVICE_OWNERS: Map<LynxServiceType, string> = new Map();
let globalModulesFrozen: boolean = false;

/** Collects one library's global registrations before they are committed. */
export class LynxLibraryRegistry {
  private packageName: string;
  private behaviors: Map<string, Behavior> = new Map();
  private modules: Map<string, ModuleClassWrapper> = new Map();
  private services: Map<LynxServiceType, IServiceProvider> = new Map();
  private initializers: LynxLibraryInitializer[] = [];

  constructor(packageName: string) {
    this.packageName = packageName;
  }

  registerBehavior(name: string, behavior: Behavior): void {
    const normalizedName = this.assertNameAvailable(
      name,
      this.behaviors,
      'Behavior'
    );
    this.behaviors.set(normalizedName, behavior);
  }

  registerModule(name: string, wrapper: ModuleClassWrapper): void {
    const normalizedName = this.assertNameAvailable(
      name,
      this.modules,
      'Native Module'
    );
    this.modules.set(normalizedName, wrapper);
  }

  registerService(serviceType: LynxServiceType, service: IServiceProvider): void {
    if (this.services.has(serviceType)) {
      throw new Error(
        `Duplicate Service type ${serviceType} in Lynx library ${this.packageName}`
      );
    }
    this.services.set(serviceType, service);
  }

  registerInitializer(initializer: LynxLibraryInitializer): void {
    this.initializers.push(initializer);
  }

  static setupGlobal(entries: LynxLibraryProviderEntry[]): void {
    const stagedRegistries: LynxLibraryRegistry[] = [];

    for (const entry of entries) {
      const packageName = entry.packageName.trim();
      if (packageName.length === 0) {
        throw new Error('Lynx library packageName must not be empty');
      }
      if (REGISTERED_PACKAGES.has(packageName)) {
        continue;
      }

      const registry = new LynxLibraryRegistry(packageName);
      entry.provider.register(registry);
      stagedRegistries.push(registry);
    }

    LynxLibraryRegistry.validate(stagedRegistries);

    for (const registry of stagedRegistries) {
      registry.runInitializers();
    }
    for (const registry of stagedRegistries) {
      registry.commit();
    }
  }

  private static validate(registries: LynxLibraryRegistry[]): void {
    const behaviorOwners = new Map<string, string>(BEHAVIOR_OWNERS);
    const moduleOwners = new Map<string, string>(MODULE_OWNERS);
    const serviceOwners = new Map<LynxServiceType, string>(SERVICE_OWNERS);
    const packageNames: Set<string> = new Set();

    for (const registry of registries) {
      if (
        REGISTERED_PACKAGES.has(registry.packageName)
        || packageNames.has(registry.packageName)
      ) {
        throw new Error(`Duplicate Lynx library package ${registry.packageName}`);
      }
      packageNames.add(registry.packageName);
      if (globalModulesFrozen && registry.modules.size > 0) {
        throw new Error(
          `Cannot register Native Modules from ${registry.packageName} after a Lynx Runtime has been created`
        );
      }

      registry.behaviors.forEach((_behavior, name) => {
        const owner = behaviorOwners.get(name);
        if (owner !== undefined) {
          throw new Error(
            `Duplicate global Behavior "${name}" from ${registry.packageName}; already registered by ${owner}`
          );
        }
        if (BUILTIN_BEHAVIORS.has(name)) {
          throw new Error(
            `Duplicate global Behavior "${name}" from ${registry.packageName}; already registered by Lynx`
          );
        }
        behaviorOwners.set(name, registry.packageName);
      });

      registry.modules.forEach((_module, name) => {
        const owner = moduleOwners.get(name);
        if (owner !== undefined) {
          throw new Error(
            `Duplicate global Native Module "${name}" from ${registry.packageName}; already registered by ${owner}`
          );
        }
        moduleOwners.set(name, registry.packageName);
      });

      registry.services.forEach((_service, serviceType) => {
        const owner = serviceOwners.get(serviceType);
        if (owner !== undefined) {
          throw new Error(
            `Duplicate global Service type ${serviceType} from ${registry.packageName}; already registered by ${owner}`
          );
        }
        serviceOwners.set(serviceType, registry.packageName);
      });
    }
  }

  private assertNameAvailable<T>(
    name: string,
    values: Map<string, T>,
    kind: string
  ): string {
    const normalizedName = name.trim();
    if (normalizedName.length === 0) {
      throw new Error(`${kind} name must not be empty in Lynx library ${this.packageName}`);
    }
    if (values.has(normalizedName)) {
      throw new Error(
        `Duplicate ${kind} "${normalizedName}" in Lynx library ${this.packageName}`
      );
    }
    return normalizedName;
  }

  private runInitializers(): void {
    for (const initializer of this.initializers) {
      initializer();
    }
  }

  private commit(): void {
    this.behaviors.forEach((behavior, name) => {
      BUILTIN_BEHAVIORS.set(name, behavior);
      BEHAVIOR_OWNERS.set(name, this.packageName);
    });
    this.modules.forEach((wrapper, name) => {
      GLOBAL_MODULES.set(name, wrapper);
      MODULE_OWNERS.set(name, this.packageName);
    });
    this.services.forEach((service, serviceType) => {
      LynxServiceCenter.registerService(serviceType, service);
      SERVICE_OWNERS.set(serviceType, this.packageName);
    });
    REGISTERED_PACKAGES.add(this.packageName);
  }
}

/** Registers all global modules before explicit runtime-specific modules. */
export function registerGlobalLynxModules(moduleManager: LynxModuleManager): void {
  globalModulesFrozen = true;
  GLOBAL_MODULES.forEach((wrapper, name) => {
    moduleManager.registerModule(name, wrapper);
  });
}

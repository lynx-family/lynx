// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ModuleClassWrapper } from '../../jsbridge/LynxModule';
import { Behavior, BehaviorRegistryMap } from '../behavior/Behavior';
import { IServiceProvider } from '../service/IServiceProvider';
import { LynxServiceCenter, LynxServiceType } from '../service/LynxServiceCenter';

export interface LynxLibraryProvider {
  register(registry: LynxLibraryRegistry): void;
}

export interface LynxLibraryProviderEntry {
  packageName: string;
  provider: LynxLibraryProvider;
}

const REGISTERED_PROVIDERS: Set<string> = new Set();
const GLOBAL_BEHAVIORS: Map<string, Behavior> = new Map();
const GLOBAL_MODULES: Map<string, ModuleClassWrapper> = new Map();
const BEHAVIOR_OWNERS: Map<string, string> = new Map();
const MODULE_OWNERS: Map<string, string> = new Map();
const INITIALIZER_OWNERS: Map<string, string> = new Map();
const SERVICE_OWNERS: Map<LynxServiceType, string> = new Map();

let moduleRegistrationClosed: boolean = false;

export class LynxLibraryRegistry {
  private readonly packageName: string;
  private readonly behaviors: Map<string, Behavior> = new Map();
  private readonly modules: Map<string, ModuleClassWrapper> = new Map();
  private readonly services: Map<LynxServiceType, IServiceProvider> = new Map();
  private readonly initializers: Map<string, () => void> = new Map();

  private constructor(packageName: string) {
    this.packageName = packageName;
  }

  registerBehavior(name: string, behavior: Behavior): void {
    validateName('Element', name);
    ensurePendingNameAvailable('Element', name, this.behaviors.has(name));
    this.behaviors.set(name, behavior);
  }

  registerModule(name: string, wrapper: ModuleClassWrapper): void {
    if (moduleRegistrationClosed) {
      throw new Error(`Cannot register Native Module '${name}' after a Lynx runtime has been created`);
    }
    validateName('Native Module', name);
    ensurePendingNameAvailable('Native Module', name, this.modules.has(name));
    this.modules.set(name, wrapper);
  }

  registerService(type: LynxServiceType, instance: IServiceProvider): void {
    if (!isKnownServiceType(type)) {
      throw new Error(`Unknown LynxServiceType '${type}' in ${this.packageName}`);
    }
    if (this.services.has(type)) {
      throw new Error(`Provider ${this.packageName} registered LynxServiceType '${type}' more than once`);
    }
    this.services.set(type, instance);
  }

  registerInitializer(name: string, initializer: () => void): void {
    validateName('initializer', name);
    ensurePendingNameAvailable('initializer', name, this.initializers.has(name));
    this.initializers.set(name, initializer);
  }

  static setupGlobal(entries: LynxLibraryProviderEntry[]): void {
    entries.forEach((entry: LynxLibraryProviderEntry) => {
      validateName('package', entry.packageName);
      if (REGISTERED_PROVIDERS.has(entry.packageName)) {
        return;
      }

      const registry = new LynxLibraryRegistry(entry.packageName);
      entry.provider.register(registry);
      registry.commit();
      REGISTERED_PROVIDERS.add(entry.packageName);
    });
  }

  private commit(): void {
    this.validateConflicts();

    this.initializers.forEach((initializer: () => void) => {
      initializer();
    });

    this.behaviors.forEach((behavior: Behavior, name: string) => {
      GLOBAL_BEHAVIORS.set(name, behavior);
      BEHAVIOR_OWNERS.set(name, this.packageName);
    });
    this.modules.forEach((wrapper: ModuleClassWrapper, name: string) => {
      GLOBAL_MODULES.set(name, wrapper);
      MODULE_OWNERS.set(name, this.packageName);
    });
    this.initializers.forEach((_initializer: () => void, name: string) => {
      INITIALIZER_OWNERS.set(name, this.packageName);
    });
    this.services.forEach((instance: IServiceProvider, type: LynxServiceType) => {
      LynxServiceCenter.registerService(type, instance);
      SERVICE_OWNERS.set(type, this.packageName);
    });
  }

  private validateConflicts(): void {
    this.behaviors.forEach((_behavior: Behavior, name: string) => {
      ensureGlobalNameAvailable('Element', name, this.packageName, BEHAVIOR_OWNERS);
    });
    this.modules.forEach((_wrapper: ModuleClassWrapper, name: string) => {
      ensureGlobalNameAvailable('Native Module', name, this.packageName, MODULE_OWNERS);
    });
    this.initializers.forEach((_initializer: () => void, name: string) => {
      ensureGlobalNameAvailable('initializer', name, this.packageName, INITIALIZER_OWNERS);
    });
    this.services.forEach((_instance: IServiceProvider, type: LynxServiceType) => {
      const owner = SERVICE_OWNERS.get(type);
      if (owner !== undefined) {
        throw new Error(
          `LynxServiceType '${type}' from ${this.packageName} conflicts with provider ${owner}`
        );
      }
    });
  }
}

export function mergeGlobalBehaviors(behaviors: BehaviorRegistryMap): BehaviorRegistryMap {
  const merged: BehaviorRegistryMap = new Map();
  GLOBAL_BEHAVIORS.forEach((behavior: Behavior, name: string) => {
    merged.set(name, behavior);
  });
  behaviors.forEach((behavior: Behavior, name: string) => {
    merged.set(name, behavior);
  });
  return merged;
}

export function getGlobalModuleWrappers(): Map<string, ModuleClassWrapper> {
  moduleRegistrationClosed = true;
  const modules: Map<string, ModuleClassWrapper> = new Map();
  GLOBAL_MODULES.forEach((wrapper: ModuleClassWrapper, name: string) => {
    modules.set(name, wrapper);
  });
  return modules;
}

function validateName(kind: string, name: string): void {
  if (name.trim().length === 0) {
    throw new Error(`${kind} name must not be empty`);
  }
}

function ensurePendingNameAvailable(kind: string, name: string, duplicate: boolean): void {
  if (duplicate) {
    throw new Error(`${kind} '${name}' was registered more than once by the same provider`);
  }
}

function ensureGlobalNameAvailable(
  kind: string,
  name: string,
  packageName: string,
  owners: Map<string, string>
): void {
  const owner = owners.get(name);
  if (owner !== undefined) {
    throw new Error(`${kind} '${name}' from ${packageName} conflicts with provider ${owner}`);
  }
}

function isKnownServiceType(type: LynxServiceType): boolean {
  switch (type) {
    case LynxServiceType.Trail:
    case LynxServiceType.EventReporter:
    case LynxServiceType.Http:
    case LynxServiceType.Security:
    case LynxServiceType.Extension:
    case LynxServiceType.DevTool:
    case LynxServiceType.Image:
      return true;
    default:
      return false;
  }
}

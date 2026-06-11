// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { SendableLynxModule, RuntimeLifecycleListener } from '@lynx/lynx';
import nativeNodeAPI from 'liblynx_napi_addon_loader.so';

@Sendable
export class LynxNodeAPIToken {
  private static nextId: number = 1;
  public readonly id: number;

  constructor() {
    this.id = LynxNodeAPIToken.nextId++;
  }
}

@Sendable
export class LynxNodeAPIModule extends SendableLynxModule {
  private token?: LynxNodeAPIToken;

  private static isValidEnv(env: number[]): boolean {
    return !!env && env.length === 2 && (env[0] !== 0 || env[1] !== 0);
  }

  public static putEnv(token: LynxNodeAPIToken, env: number[]) {
    if (!LynxNodeAPIModule.isValidEnv(env)) {
      nativeNodeAPI.removeEnvByToken(token.id);
      return;
    }
    nativeNodeAPI.putEnvByToken(token.id, env);
  }

  public static removeEnv(token: LynxNodeAPIToken) {
    nativeNodeAPI.removeEnvByToken(token.id);
  }

  constructor(param?: LynxNodeAPIToken) {
    super(param);
    this.token = param;
  }

  public requireNodeAddon(addonName: string): void {
    const token = this.token;
    if (!token) {
      return;
    }
    nativeNodeAPI.requireNodeAddonByToken(token.id, addonName);
  }
}

export class LynxNodeAPILifecycleListener implements RuntimeLifecycleListener {
  private token: LynxNodeAPIToken;

  constructor(token: LynxNodeAPIToken) {
    this.token = token;
  }

  onRuntimeAttach(napiEnv: number[]): void {
    LynxNodeAPIModule.putEnv(this.token, napiEnv);
  }

  onRuntimeDetach(): void {
    LynxNodeAPIModule.removeEnv(this.token);
  }
}

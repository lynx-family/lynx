// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class TemplateBundleOption {
  private contextPoolSize: number = 0;
  private enableContextAutoRefill: boolean = false;

  constructor(contextPoolSize: number, enableContextAutoRefill: boolean) {
    this.contextPoolSize = contextPoolSize;
    this.enableContextAutoRefill = enableContextAutoRefill;
  }

  public getContextPoolSize(): number {
    return this.contextPoolSize;
  }

  public getEnableContextAutoRefill(): boolean {
    return this.enableContextAutoRefill;
  }
}
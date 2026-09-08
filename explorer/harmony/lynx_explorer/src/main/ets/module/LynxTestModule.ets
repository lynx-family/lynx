// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxContext, LynxModule, LynxUpdateMeta, LynxUpdateMode, MetaData, TemplateData } from '@lynx/lynx';

interface TestResult {
  result: string;
}

export class LynxTestModuleHost {
  active: boolean = true;
  addButton: (info: Record<string, Object>) => void;
  back: () => void;

  constructor(addButton: (info: Record<string, Object>) => void, back: () => void) {
    this.addButton = addButton;
    this.back = back;
  }
}

export class LynxTestButton {
  id: number;
  count: number;
  left: number;
  top: number;
  fontSize: number;

  constructor(id: number, info: Record<string, Object>) {
    this.id = id;
    this.count = (info.count as number) ?? 0;
    this.left = (info.left as number) ?? 0;
    this.top = (info.top as number) ?? 0;
    this.fontSize = (info.fontSize as number) ?? 16;
  }
}

// Only bridge methods live on this prototype: the module registry enumerates it.
export class LynxTestModule extends LynxModule {
  static syncMethods: string[] = ['callSync', 'invokeSync'];
  private readonly host?: LynxTestModuleHost;

  constructor(context: LynxContext, param?: Object) {
    super(context, param);
    this.host = param as LynxTestModuleHost | undefined;
  }

  public call(name: string, data: Record<string, Object>, callback: (data: Object) => void): void {
    this.invoke(data, callback);
  }

  public invoke(data: Record<string, Object>, callback: (data: Object) => void): void {
    setTimeout(() => {
      if (this.host?.active && callback) {
        const result: TestResult = { result: 'success' };
        callback(result);
      }
    }, 100);
  }

  public callSync(name: string, data: Record<string, Object>): TestResult {
    return this.invokeSync(data);
  }

  public invokeSync(data: Record<string, Object>): TestResult {
    return { result: '----lepus value--success' };
  }

  public updateData(data: Record<string, Object>): void {
    if (this.host?.active) {
      this.context.updateMetaData(new LynxUpdateMeta(new TemplateData(data)));
    }
  }

  public resetData(data: Record<string, Object>): void {
    if (this.host?.active) {
      this.context.updateMetaData(new LynxUpdateMeta(new TemplateData(data), undefined, LynxUpdateMode.RESET));
    }
  }

  public updateGlobalProps(props: Record<string, Object>): void {
    if (this.host?.active) {
      this.context.updateMetaData(new LynxUpdateMeta(undefined, new TemplateData(props)));
    }
  }

  public reloadTemplate(data: Record<string, Object>, props: Record<string, Object>): void {
    if (this.host?.active) {
      this.context.reload(new MetaData(new TemplateData(data), new TemplateData(props)));
    }
  }

  public reload(): void {
    if (this.host?.active) {
      this.context.loadTemplate(undefined, undefined, undefined, new MetaData(new TemplateData({})));
    }
  }

  public getPageDataByKey(keys: string[], callback: (data: Object | null) => void): void {
    if (this.host?.active && callback) {
      this.context.getPageDataByKeyAsync(keys, (data: Object | null) => {
        if (this.host?.active) {
          callback(data);
        }
      });
    }
  }

  public updateScreenMatrix(matrix: Record<string, number>): void {
    if (this.host?.active) {
      this.context.updateScreenMetrics(matrix.width, matrix.height);
    }
  }

  public addButton(info: Record<string, Object>): void {
    if (this.host?.active) {
      this.host.addButton(info);
    }
  }

  public eventTest(value: string): void {
    if (this.host?.active) {
      this.context.sendGlobalEvent('test', [10, value]);
    }
  }

  public valueTest(value: string): void {
    if (!this.host?.active) {
      return;
    }
    let result: Object = value;
    try {
      const parsed: Object | null = JSON.parse(value) as Object | null;
      if (parsed !== null && typeof parsed === 'object') {
        result = parsed;
      }
    } catch (_) {
      // Non-JSON values are sent unchanged.
    }
    this.context.sendGlobalEvent('test', [result]);
  }

  public back(): void {
    if (this.host?.active) {
      this.host.back();
    }
  }
}

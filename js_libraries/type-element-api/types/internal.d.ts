// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Complete Element API surface for Lynx runtime and framework internals.
 *
 * This tier includes every experimental and stable API. It also declares
 * runtime plumbing that is retained at the package root for compatibility.
 */

import type { ElementRef, ForElementRef, IfElementRef } from './shared';

export * from './experimental';

export interface PipelineOptions {
  pipelineID: string;
  needTimestamps: boolean;
}

export interface DynamicComponentResult {
  code: number;
  data: {
    evalResult: (
      url: string
    ) => {
      name: string;
    };
  };
}

declare global {
  function __UpdateComponentID(node: ElementRef, id: string): void;

  function __UpdateComponentInfo(
    node: ElementRef,
    params: {
      componentID?: string;
      name?: string;
      path?: string;
      entry?: string;
      cssID?: number;
      config?: Record<string, unknown>;
    }
  ): void;

  function __FlushElementTree(
    element?: ElementRef,
    options?: {
      triggerLayout?: boolean;
      triggerDataUpdated?: boolean;
      operationID?: number;
      nativeUpdateDataOrder?: number;
      __lynx_timing_flag?: string;
      elementID?: number;
      reloadTemplate?: boolean;
      listID?: number;
      pipelineOptions?: Record<string, any>;
      elementIDs?: number[];
      operationIDs?: number[];
      asyncFlush?: boolean;
      onLayoutReady?: () => void;
      emptyPatch?: boolean;
    }
  ): void;

  function __AsyncResolveElement(element: ElementRef): void;

  function __AsyncResolveSubtree(node: ElementRef): void;

  function __OnLifecycleEvent(args: any[]): void;

  function _ReportError(
    err: Error,
    info: {
      errorCode: number;
    }
  ): void;

  function __ElementFromBinary(elementTemplateKey: string, parentComponentUniId: number): ElementRef[];

  function __GetTemplateParts(ele: ElementRef): Record<string, ElementRef>;

  function __CloneElement(ele: ElementRef, options: Record<string, any>): ElementRef;

  function __IsTemplateElement(ele: ElementRef): boolean;

  function __MarkTemplateElement(ele: ElementRef): void;

  function __MarkPartElement(ele: ElementRef, key: string): void;

  function __AddConfig(ele: ElementRef, key: string, value: any): void;

  function __SetConfig(ele: ElementRef, config: Record<string, any>): void;

  function __GetConfig(ele: ElementRef): Record<string, unknown>;

  function __QueryComponent(source: string, callback?: (evalResult: DynamicComponentResult) => void): { evalResult: unknown };

  function __InvokeUIMethod(e: ElementRef, method: string, params: Record<string, unknown>, callback: (res: { code: number; data: unknown }) => void): ElementRef[];

  function __LoadLepusChunk(name: string, cfg: { chunkType: number }): void;

  function __GeneratePipelineOptions(): Record<string, any>;

  function __OnPipelineStart(pipeLineId: string, pipeLineOrigin: string): void;

  function __BindPipelineIDWithTimingFlag(pipeLineId: string, timingFlag: string): void;

  function __MarkTiming(pipeLineId: string, timingFlag: string): void;

  function __AddTimingListener(): void;

  function __SetLepusInitData(initData: Object): void;

  function __UpdateIfNodeIndex(node: IfElementRef, ifIndex: number): void;

  function __UpdateForChildCount(node: ForElementRef, childCount: number): void;
}

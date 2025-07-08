// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxError } from './LynxError';
import { LynxContext } from './LynxContext';
import { TemplateBundle } from './TemplateBundle';
import { PerformanceEntry } from './gen/performance/performanceobserver/PerformanceEntry';
import { IPerformanceObserver } from './performance/IPerformanceObserver';

/**
 * The cause that the lynx pixel pipeline is activated
 */
export enum LynxPipelineOrigin {
  LynxFirstScreen = 1,
  LynxReload = 1 << 1,
}

/**
 * Provide information about the lynx pixel pipeline
 */
export class LynxPipelineInfo {
  /**
   * url of LynxView
   */
  public url: string = '';
  /**
   * origin of pixel pipeline in Lynx lifecycle
   */
  private pipelineOrigin: number = 0;

  constructor(url: string) {
    this.url = url;
  }

  public addPipelineOrigin(pipelineOrigin: LynxPipelineOrigin): void {
    this.pipelineOrigin |= pipelineOrigin;
  }

  /**
   * @return whether the pixel pipeline is caused by first screen
   */
  public isFromFirstScreen(): boolean {
    return (this.pipelineOrigin & LynxPipelineOrigin.LynxFirstScreen) > 0;
  }

  /**
   * @return whether the pixel pipeline is caused by reload
   */
  public isFromReload(): boolean {
    return (this.pipelineOrigin & LynxPipelineOrigin.LynxReload) > 0;
  }

  toString(): string {
    return `LynxPipelineInfo(url: ${this.url}, pipelineOrigin: ${this.pipelineOrigin.toString(2)})`;
  }
}

/**
 * Give the host application a chance to take control when a lynx template is about to be loaded in
 * the current LynxView.
 */
export abstract class LynxViewClient implements IPerformanceObserver {
  /**
   * Notify that a lynx template has started loading. It will be call at both `loadTemplate` and
   * `reloadTemplate`.
   *
   * Note: this method will be executed before the main process of lynx so do not execute overly
   * complex logic in this method.
   *
   * @param lynxView the LynxView which has started loading
   * @param pipelineInfo the information about the pixel pipeline
   *
   */
  public onPageStarted(lynxContext: LynxContext, pipelineInfo: LynxPipelineInfo): void {
  }

  public onLoadSuccess(url: string): void {
  }

  public onFirstScreen(): void {
  }

  public onPageUpdate(): void {
  }

  public onDataUpdated(): void {
  }

  public onRuntimeReady(): void {
  }

  public onReceivedError(error: LynxError): void {
  }

  public onDestroy(): void {
  }

  public onUpdateDataWithoutChange(): void {
  }

  public onTemplateBundleReady(bundle: TemplateBundle): void {
  }

  public onPerformanceEvent(entry: PerformanceEntry): void {
  }
}
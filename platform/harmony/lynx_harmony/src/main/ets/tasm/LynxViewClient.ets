// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxError } from './LynxError';
import { LynxContext } from './LynxContext';
import { TemplateBundle } from './TemplateBundle';
import { PerformanceEntry } from './gen/performance/performanceobserver/PerformanceEntry';
import { IPerformanceObserver } from './performance/IPerformanceObserver';
import { LynxResourceType } from '../Index';

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
 * Provide information about the loaded resource.
 */
export interface LynxResourceLoadInfo {
  /**
   * The type of the resource.
   */
  type: LynxResourceType;

  /**
   * The error code of the resource loading.
   */
  errCode: number;

  /**
   * The error message of the resource loading.
   */
  errMsg?: string;
}

/**
 * A client that provides callbacks for `LynxView`'s lifecycle and other events.
 * @apidoc
 */
export abstract class LynxViewClient implements IPerformanceObserver {
  /**
   * Called when page start loading.
   * @apidoc
   * @note This method will be executed before the main process of lynx so do not execute overly
   * complex logic in this method.
   *
   * @param lynxContext The LnxContext which has started loading
   * @param pipelineInfo The information about the pixel pipeline
   *
   */
  public onPageStarted(lynxContext: LynxContext, pipelineInfo: LynxPipelineInfo): void {
  }

  /**
   * Called when page load finish.
   * @apidoc
   * @param url The page url.
   */
  public onLoadSuccess(url: string): void {
  }

  /**
   * Called when first screen layout completed.
   * @apidoc
   */
  public onFirstScreen(): void {
  }

  /**
   * Called when page update.
   * @apidoc
   */
  public onPageUpdate(): void {
  }

  /**
   * Called when data update, but the view may not be updated.
   * @apidoc
   */
  public onDataUpdated(): void {
  }

  /**
   * Called when JS environment preparation completed.
   * @apidoc
   */
  public onRuntimeReady(): void {
  }

  /**
   * Called when error received.
   * @apidoc
   * @param error The type and information of the error.
   */
  public onReceivedError(error: LynxError): void {
  }

  /**
   * Called after the page is destroyed.
   * @apidoc
   */
  public onDestroy(): void {
  }

  /**
   * If there is no need to update the UI after calling
   * [`updateData`](../lynx-view/update-data.mdx), this method is called back.
   * @apidoc
   */
  public onUpdateDataWithoutChange(): void {
  }

  /**
   * Called when the intrinsic content size of LynxView changes.
   * @apidoc
   * @param width The intrinsic content width.
   * @param height The intrinsic content height.
   */
  public onIntrinsicContentSizeChange(width: number, height: number): void {
  }

  /**
   * Provide a reusable TemplateBundle after template is decode.
   * @apidoc
   * @note This callback is disabled by default, and you can enable it through the
   * DUMP_ELEMENT option or RECYCLE_TEMPLATE_BUNDLE option in LynxLoadMeta.
   * @param bundle The recycled template bundle, it is nonnull but could be invalid.
   */
  public onTemplateBundleReady(bundle: TemplateBundle): void {
  }

  /**
   * Notify the client that a performance event has been sent. It will be called every time a
   * performance event is generated, including but not limited to container initialization, engine
   * rendering, rendering metrics update, etc.
   *
   * @apidoc
   * @param entry The [PerformanceEntry](../../lynx-api/performance-api/performance-entry.mdx)
   * about the performance event.
   *
   */
  public onPerformanceEvent(entry: PerformanceEntry): void {
  }

  /**
   * Notify the client that a resource has been loaded.
   * @apidoc
   * @param result The information about the loaded resource.
   */
  public onResourceLoaded(result: LynxResourceLoadInfo): void {
  }
}

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  LLog,
  LynxContext,
  LynxError,
  LynxPipelineInfo,
  LynxViewClient,
  PerformanceEntry,
  TemplateBundle
} from '@lynx/lynx';

const TAG = 'EntryLynxClient';

export class EntryLynxClient extends LynxViewClient {
  override onPageStarted(lynxContext: LynxContext, pipelineInfo: LynxPipelineInfo): void {
    LLog.d(TAG, 'onPageStarted: ' + pipelineInfo);
  }

  public onLoadSuccess(url: string): void {
    LLog.d(TAG, 'onLoadSuccess: ' + url);
  }

  public onFirstScreen(): void {
    LLog.d(TAG, 'onFirstScreen');
  }

  public onPageUpdate(): void {
    LLog.d(TAG, 'onPageUpdate');
  }

  public onDataUpdated(): void {
    LLog.d(TAG, 'onDataUpdated');
  }

  public onRuntimeReady(): void {
    LLog.d(TAG, 'onRuntimeReady');
  }

  public onReceivedError(error: LynxError): void {
    LLog.d(TAG, 'onReceivedError: ' + error.getMsg());
  }

  public onDestroy(): void {
  }

  public onUpdateDataWithoutChange(): void {
  }

  public onTemplateBundleReady(bundle: TemplateBundle): void {
    LLog.d(TAG, `onTemplateBundleReady`);
  }

  public onPerformanceEvent(entry: PerformanceEntry): void {
    LLog.d(TAG, `onPerformanceEvent` + entry.toRecord());
  }
}
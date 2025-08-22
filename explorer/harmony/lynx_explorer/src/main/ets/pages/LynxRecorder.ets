// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import router from '@ohos.router';
import { LynxGenericResourceFetcher, LynxMediaResourceFetcher, LynxTemplateResourceFetcher, } from '@lynx/lynx';
import { ExampleGenericResourceFetcher } from '../provider/ExampleGenericResourceFetcher';
import { ExampleMediaResourceFetcher } from '../provider/ExampleMediaResourceFetcher';
import { ExampleTemplateResourceFetcher } from '../provider/ExampleTemplateResourceFetcher';
import { LynxRecorderView } from '@lynx/lynx_devtool';


interface LynxRecorderParams {
  url: string;
}

@Entry
@Component
struct LynxRecorder {
  private url: string = (router.getParams() as LynxRecorderParams).url;
  private genericFetcher?: LynxGenericResourceFetcher = new ExampleGenericResourceFetcher();
  private mediaFetcher?: LynxMediaResourceFetcher = new ExampleMediaResourceFetcher();
  private templateFetcher?: LynxTemplateResourceFetcher = new ExampleTemplateResourceFetcher();

  aboutToDisappear(): void {
    this.genericFetcher = undefined;
    this.mediaFetcher = undefined;
    this.templateFetcher = undefined;
  }

  build() {
    Column() {
      LynxRecorderView({
        url: this.url,
        genericFetcher: this.genericFetcher,
        mediaFetcher: this.mediaFetcher,
        templateFetcher: this.templateFetcher,
        uiRendererCreator: undefined
      })
    }.size({ width: '100%', height: '100%' })
  }
}

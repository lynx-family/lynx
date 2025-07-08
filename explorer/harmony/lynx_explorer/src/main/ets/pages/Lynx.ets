// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import router from '@ohos.router';
import url from '@ohos.url';
import { display } from '@kit.ArkUI';
import {
  Behavior,
  LLog,
  LynxContext,
  LynxExtraTiming,
  LynxGenericResourceFetcher,
  LynxMediaResourceFetcher,
  LynxTemplateResourceFetcher,
  LynxView,
  MetaData,
  ModuleClassWrapper,
  SendableModuleClassWrapper,
  TemplateData
} from '@lynx/lynx';
import Constants from '../common/constants';
import { UIClickView } from '../component/UIClickView';
import { ClickShadowNode } from '../component/UIClickShadowNode';
import { EntryLynxClient } from '../client/EntryLynxClient';
import { ExampleGenericResourceFetcher } from '../provider/ExampleGenericResourceFetcher';
import { ExampleMediaResourceFetcher } from '../provider/ExampleMediaResourceFetcher';
import { ExampleTemplateResourceFetcher } from '../provider/ExampleTemplateResourceFetcher';
import { ExplorerModule } from '../module/ExplorerModule';

const TAG = 'LynxPage'
const TEST_DENSITY = 2;

interface LynxParam {
  url: string;
}

class Param {
  url: string;

  constructor(url: string) {
    this.url = url;
  }
}

@Entry
@Component
struct Lynx {
  @State count: number = 0;
  private url: string = (router.getParams() as LynxParam).url;
  private modules: Map<string, ModuleClassWrapper> = new Map();
  private sendableModules: Map<string, SendableModuleClassWrapper> = new Map();
  private genericFetcher: LynxGenericResourceFetcher = new ExampleGenericResourceFetcher();
  private mediaFetcher: LynxMediaResourceFetcher = new ExampleMediaResourceFetcher();
  private templateFetcher: LynxTemplateResourceFetcher = new ExampleTemplateResourceFetcher();
  private metaData = new MetaData(
    new TemplateData('{"text":"TemplateData"}'),
    new TemplateData({ "text": "Global" }));
  private clients: EntryLynxClient[] = [new EntryLynxClient()];
  private viewHeight: number | string = '95%';
  private viewWidth: number | string = '100%';
  private viewDensity = display.getDefaultDisplaySync().densityPixels;
  private heightPixel: number = 0;
  private widthPixel: number = 0;
  private extraTiming: LynxExtraTiming = new LynxExtraTiming();

  aboutToAppear() {
    this.modules.set('ExplorerModule', {
      moduleClass: ExplorerModule,
    });
    if (this.url.startsWith(Constants.SSL_LOCAL)) {
      this.url = this.url.substring(Constants.SSL_LOCAL.length);
    }
    if (this.url.startsWith(Constants.SSL_LOCAL_UITEST)) {
      this.url = this.url.substring(Constants.SSL_LOCAL_UITEST.length);
    }
    try {
      let urlObj = url.URL.parseURL(this.url);
      let queryParams = new url.URLParams(urlObj.search);
      if (this.url.startsWith(Constants.LOCAL)) {
        this.url = this.url.substring(Constants.LOCAL.length);
      } else if (this.url.startsWith(Constants.FILE_LOCAL)) {
        this.url = this.url.substring(Constants.FILE_LOCAL.length);
      }
      let index = this.url.indexOf('?');
      if (index !== -1) {
        this.url = this.url.slice(0, index);
      }
      if (queryParams.get('height') !== undefined) {
        this.heightPixel = parseInt(queryParams.get('height') as string);
        this.viewHeight = this.heightPixel / TEST_DENSITY;
      }
      if (queryParams.get('width') !== undefined) {
        this.widthPixel = parseInt(queryParams.get('width') as string);
        this.viewWidth = this.widthPixel / TEST_DENSITY;
      }
    } catch (e) {
      LLog.e(TAG, `url ${this.url} parsed failed: ${e}`);
    }

    // mock extraTiming here
    this.extraTiming.openTime = new Date().getTime();
    this.extraTiming.containerInitStart = new Date().getTime();
    this.extraTiming.containerInitEnd = new Date().getTime();
    this.extraTiming.prepareTemplateStart = new Date().getTime();
    this.extraTiming.prepareTemplateEnd = new Date().getTime();
  }

  build() {
    Column() {
      Text('Lynx').fontSize(18).width('100%').height('5%').textAlign(TextAlign.Center)

      LynxView({
        clients: this.clients,
        url: this.url,
        genericResourceFetcher: this.genericFetcher,
        mediaResourceFetcher: this.mediaFetcher,
        templateResourceFetcher: this.templateFetcher,
        modules: this.modules,
        sendableModules: this.sendableModules,
        metaData: this.metaData,
        onCreate: (context: LynxContext) => {
          let param: Param = new Param(this.url);
          context.sendGlobalEvent('viewAppear', [param]);
          if (this.widthPixel > 0 && this.heightPixel > 0) {
            context.updateScreenMetrics(this.widthPixel * this.viewDensity / TEST_DENSITY,
              this.heightPixel * this.viewDensity / TEST_DENSITY);
          }
          context.setExtraTiming(this.extraTiming);
        }
      }).height(this.viewHeight).width(this.viewWidth)
    }.size({ width: '100%', height: '100%' }).alignItems(HorizontalAlign.Start)
  }
}

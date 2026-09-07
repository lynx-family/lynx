// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import router from '@ohos.router';
import url from '@ohos.url';
import { display, window } from '@kit.ArkUI';
import { common } from '@kit.AbilityKit';
import {
  Behavior,
  BehaviorRegistryMap,
  LLog,
  LynxBackgroundRuntime,
  LynxBackgroundRuntimeOptions,
  LynxContext,
  LynxExtraTiming,
  LynxGenericResourceFetcher,
  LynxGroup,
  LynxMediaResourceFetcher,
  LynxResourceRequest,
  LynxResourceType,
  LynxTemplateResourceFetcher,
  LynxView,
  MetaData,
  ModuleClassWrapper,
  SendableModuleClassWrapper,
  TemplateBundle,
  TemplateData
} from '@lynx/lynx';
import { BusinessError } from '@ohos.base';
import util from '@ohos.util';
import { UISVG } from '@lynx/xelement_svg';
import { UIWebView } from '@lynx/xelement_webview';
import { UIVideo } from '@lynx/xelement_video';
import Constants from '../common/constants';
import { EntryLynxClient } from '../client/EntryLynxClient';
import { ExampleGenericResourceFetcher } from '../provider/ExampleGenericResourceFetcher';
import { ExampleMediaResourceFetcher } from '../provider/ExampleMediaResourceFetcher';
import { ExampleTemplateResourceFetcher } from '../provider/ExampleTemplateResourceFetcher';
import { ExplorerModule } from '../module/ExplorerModule';
import { JSBTestModule } from '../module/JSBTestModule';
import { LynxTestButton, LynxTestModule, LynxTestModuleHost } from '../module/LynxTestModule';
import { LynxNodeAPIModule, LynxNodeAPIToken } from '../module/LynxNodeAPIModule';

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
  private behaviors: BehaviorRegistryMap = new Map([
    ['svg',  new Behavior(UISVG, undefined)],
    ['webview', new Behavior(UIWebView, undefined)],
    ['video', new Behavior(UIVideo, undefined)]]);
  private nodeAPIToken: LynxNodeAPIToken = new LynxNodeAPIToken();
  private backgroundRuntime?: LynxBackgroundRuntime;
  private enableNapiAddon: boolean = false;
  private standaloneUrl?: string;
  private standaloneEntryName?: string;
  private lynxContext?: LynxContext;
  private windowStageEventCallback?: Callback<window.WindowStageEventType>;
  private listenerWindowStage?: window.WindowStage;
  @State testButtons: LynxTestButton[] = [];
  private nextTestButtonId: number = 0;
  private routeIndex: number = 0;
  private testModuleHost: LynxTestModuleHost = new LynxTestModuleHost((info) => {
    this.testButtons = this.testButtons.concat(new LynxTestButton(this.nextTestButtonId++, info));
  }, () => {
    if (router.getState().index === this.routeIndex) {
      router.back();
    }
  });
  @State isWaitingBackgroundRuntime: boolean = false;
  @StorageLink('lynxWindowStage') windowStage: window.WindowStage | undefined = undefined;
  
  aboutToAppear() {
    this.routeIndex = router.getState().index;
    this.testModuleHost.active = true;
    this.registerWindowStageEventListener();
    this.modules.set('ExplorerModule', {
      moduleClass: ExplorerModule,
      param: getContext(this) as common.UIAbilityContext
    });
    this.modules.set('JSBTestModule', { moduleClass: JSBTestModule });
    this.modules.set('bridge', { moduleClass: JSBTestModule });
    this.modules.set('LynxTestModule', { moduleClass: LynxTestModule, param: this.testModuleHost });
    this.sendableModules.set('LynxNodeAPI',
      new SendableModuleClassWrapper(LynxNodeAPIModule, this.nodeAPIToken));
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
      let enableNapiAddon = queryParams.get('enable_napi_addon');
      this.enableNapiAddon = enableNapiAddon === '1' || enableNapiAddon === 'true';
      const standaloneUrl = queryParams.get('standalone_url');
      this.standaloneUrl = standaloneUrl === null ? undefined : standaloneUrl;
      const standaloneEntryName = queryParams.get('standalone_entry_name');
      this.standaloneEntryName = standaloneEntryName === null ? undefined : standaloneEntryName;
    } catch (e) {
      LLog.e(TAG, `url ${this.url} parsed failed: ${e}`);
    }

    // mock extraTiming here
    this.extraTiming.openTime = new Date().getTime();
    this.extraTiming.containerInitStart = new Date().getTime();
    this.extraTiming.containerInitEnd = new Date().getTime();
    this.extraTiming.prepareTemplateStart = new Date().getTime();
    this.extraTiming.prepareTemplateEnd = new Date().getTime();

    this.tryPrepareBackgroundRuntime();
  }

  aboutToDisappear(): void {
    this.testModuleHost.active = false;
    this.testButtons = [];
    const callback = this.windowStageEventCallback;
    if (callback) {
      try {
        this.listenerWindowStage?.off('windowStageEvent', callback);
      } catch (e) {
        LLog.e(TAG, `Failed to remove the window stage event listener: ${e}`);
      }
    }
    this.windowStageEventCallback = undefined;
    this.listenerWindowStage = undefined;
    this.lynxContext = undefined;
  }

  private registerWindowStageEventListener(): void {
    const windowStage = this.windowStage;
    if (this.windowStageEventCallback || !windowStage) {
      return;
    }
    const callback: Callback<window.WindowStageEventType> = (event) => {
      if (event === window.WindowStageEventType.SHOWN) {
        this.lynxContext?.onEnterForeground();
      } else if (event === window.WindowStageEventType.HIDDEN) {
        this.lynxContext?.onEnterBackground();
      }
    };
    try {
      windowStage.on('windowStageEvent', callback);
      this.windowStageEventCallback = callback;
      this.listenerWindowStage = windowStage;
    } catch (e) {
      LLog.e(TAG, `Failed to add the window stage event listener: ${e}`);
    }
  }

  private tryPrepareBackgroundRuntime(): void {
    if (!this.enableNapiAddon && !this.standaloneUrl) {
      return;
    }
    try {
      let group: LynxGroup = LynxGroup.createSharedLynxGroup();
      let options: LynxBackgroundRuntimeOptions = new LynxBackgroundRuntimeOptions();
      options.lynxGroup = group;
      const backgroundModules: Map<string, ModuleClassWrapper> = new Map();
      backgroundModules.set('JSBTestModule', { moduleClass: JSBTestModule });
      this.backgroundRuntime = new LynxBackgroundRuntime(options, this.genericFetcher,
        this.mediaFetcher, this.templateFetcher, backgroundModules, this.sendableModules, this.metaData);
    } catch (e) {
      LLog.e(TAG, `Failed to create background runtime: ${e}`);
      this.backgroundRuntime = undefined;
      this.isWaitingBackgroundRuntime = false;
      return;
    }
    if (this.standaloneUrl) {
      this.isWaitingBackgroundRuntime = true;
      this.fetchAndEvaluateStandaloneScript(this.standaloneUrl);
    }
  }

  private fetchAndEvaluateStandaloneScript(standaloneUrl: string): void {
    try {
      if (standaloneUrl.startsWith('http://') || standaloneUrl.startsWith('https://')) {
        this.genericFetcher.fetchResource(
          new LynxResourceRequest(standaloneUrl, LynxResourceType.LYNX_RESOURCE_TYPE_GENERIC),
          (err: BusinessError, res: ArrayBuffer) => {
            this.evaluateStandaloneScript(err, res);
          });
      } else if (standaloneUrl.startsWith(Constants.LOCAL)) {
        const resourceMgr = (getContext(this) as common.UIAbilityContext).resourceManager;
        resourceMgr.getRawFileContent(standaloneUrl.substring(Constants.LOCAL.length),
          (err: BusinessError, data: Uint8Array) => {
            this.evaluateStandaloneScript(err, data?.buffer as ArrayBuffer);
          });
      } else {
        LLog.e(TAG, `Unsupported standalone script URL: ${standaloneUrl}`);
        this.finishStandalonePreparation(false);
      }
    } catch (e) {
      LLog.e(TAG, `Failed to fetch standalone script ${standaloneUrl}: ${e}`);
      this.finishStandalonePreparation(false);
    }
  }

  private evaluateStandaloneScript(err: BusinessError, res: ArrayBuffer): void {
    if (err) {
      LLog.e(TAG, `Failed to fetch standalone script ${this.standaloneUrl}: ` +
        `code=${err.code}, message=${err.message}`);
      this.finishStandalonePreparation(false);
      return;
    }
    if (!this.backgroundRuntime || !this.standaloneUrl || !res) {
      LLog.e(TAG, `Standalone script cannot be evaluated: ${this.standaloneUrl}`);
      this.finishStandalonePreparation(false);
      return;
    }
    try {
      if (this.standaloneEntryName) {
        const bundle = TemplateBundle.fromTemplate(res);
        this.backgroundRuntime.evaluateBundleScript(this.standaloneUrl, bundle, this.standaloneEntryName);
      } else {
        const decoder = util.TextDecoder.create('utf-8', { ignoreBOM: true });
        const script = decoder.decodeToString(new Uint8Array(res));
        this.backgroundRuntime.evaluateJavaScript(this.standaloneUrl, script);
      }
    } catch (e) {
      LLog.e(TAG, `Failed to evaluate standalone script ${this.standaloneUrl}: ${e}`);
      this.finishStandalonePreparation(false);
      return;
    }
    this.finishStandalonePreparation(true);
  }

  private finishStandalonePreparation(succeeded: boolean): void {
    if (!succeeded && !this.enableNapiAddon) {
      this.backgroundRuntime = undefined;
    }
    this.isWaitingBackgroundRuntime = false;
  }

  build() {
    Column() {
      Text('Lynx').fontSize(18).width('100%').height('5%').textAlign(TextAlign.Center)

      if (!this.isWaitingBackgroundRuntime) {
        Stack({ alignContent: Alignment.TopStart }) {
          LynxView({
            clients: this.clients,
            url: this.url,
            genericResourceFetcher: this.genericFetcher,
            mediaResourceFetcher: this.mediaFetcher,
            templateResourceFetcher: this.templateFetcher,
            backgroundRuntime: this.backgroundRuntime,
            modules: this.modules,
            sendableModules: this.sendableModules,
            behaviors: this.behaviors,
            metaData: this.metaData,
            onCreate: (context: LynxContext) => {
              this.lynxContext = context;
              let param: Param = new Param(this.url);
              context.sendGlobalEvent('viewAppear', [param]);
              if (this.widthPixel > 0 && this.heightPixel > 0) {
                context.updateScreenMetrics(this.widthPixel * this.viewDensity / TEST_DENSITY,
                  this.heightPixel * this.viewDensity / TEST_DENSITY);
              }
              context.setExtraTiming(this.extraTiming);
            }
          }).height('100%').width('100%')
          ForEach(this.testButtons, (button: LynxTestButton) => {
            Button(button.count.toString())
              .fontSize(button.fontSize)
              .position({ x: button.left, y: button.top })
              .onClick(() => {
                button.count++;
                this.testButtons = this.testButtons.slice();
              })
          }, (button: LynxTestButton) => `${button.id}:${button.count}`)
        }.height(this.viewHeight).width(this.viewWidth)
      } else {
        Text('Loading...').fontSize(30)
      }
    }.size({ width: '100%', height: '100%' }).alignItems(HorizontalAlign.Start)
  }
}

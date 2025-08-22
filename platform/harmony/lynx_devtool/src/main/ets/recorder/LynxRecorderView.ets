// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import http from '@ohos.net.http';
import { BusinessError } from '@ohos.base';
import Url from '@ohos.url';
import {
  LynxContext,
  LynxView,
  MetaData,
  ModuleClassWrapper,
  TemplateData,
  LynxEnv,
  LynxGenericResourceFetcher,
  LynxMediaResourceFetcher,
  LynxTemplateResourceFetcher,
  LLog,
  LynxExtraTiming,
  LynxUIRendererCreator
} from '@lynx/lynx';
import LynxRecorderReplayDataModule from './ReplayDataModule';
import { LoadTemplateParams, SendGlobalEventParams, SetGlobalPropsParams, UpdateViewPortParams } from './DataType';
import { util } from '@kit.ArkTS';
import LynxRecorderEnv from './LynxRecorderEnv';

const TAG = 'LynxRecorderView';

@Component
export default struct LynxRecorderView {
  public url?: string;
  public genericFetcher?: LynxGenericResourceFetcher;
  public mediaFetcher?: LynxMediaResourceFetcher;
  public templateFetcher?: LynxTemplateResourceFetcher;
  public uiRendererCreator?: LynxUIRendererCreator;
  private data: string = '';
  private pageUrl: string = LynxRecorderEnv.RECORDER_URL_PREFIX;
  private buffer: ArrayBuffer | undefined;
  private bufferReady: boolean = false;
  private templateData: TemplateData | undefined;
  private templateDataReady: boolean = false;
  private globalProps: TemplateData | undefined;
  private jsbModule: Map<string, ModuleClassWrapper> = new Map();
  private jsbModuleReady: boolean = false;
  @State ready: boolean = false;
  private localLynxContext: LynxContext | undefined;
  private extraTiming: LynxExtraTiming = new LynxExtraTiming();

  private download() {
    let httpRequest = http.createHttp();
    let urlObject = Url.URL.parseURL(this.url);
    let url = urlObject.params.get('url');
    if (url == null) {
      LLog.e(TAG, 'Unsupported artifact link：' + this.url);
      return;
    }

    let res = new ArrayBuffer(0);
    httpRequest.on('dataReceive', (data: ArrayBuffer) => {
      const newRes = new ArrayBuffer(res.byteLength + data.byteLength);
      const resView = new Uint8Array(newRes);
      resView.set(new Uint8Array(res));
      resView.set(new Uint8Array(data), res.byteLength);
      res = newRes;
    });

    httpRequest.requestInStream(
      url, {
      expectDataType: http.HttpDataType.STRING,
    }, (err: BusinessError, data: number) => {
      if (data != 200) {
        LLog.e(TAG, 'Unsupported artifact link：' + this.url);
      } else {
        let decoder = util.TextDecoder.create('utf-8');
        let stringResult = decoder.decodeToString(new Uint8Array(res));
        if (stringResult.startsWith('{')) {
          this.data = stringResult;
        } else {
          this.data = LynxEnv.parserTestBenchRecordData(stringResult);
        }
        this.registerJSBModule();
        this.handleActionList();
      }
      httpRequest.off('dataReceive');
      httpRequest.destroy();
    });
  }

  private changeReadyState() {
    this.ready = this.bufferReady && this.templateDataReady && this.jsbModuleReady;
  }

  aboutToDisappear(): void {
    if (LynxRecorderReplayDataModule.jsbInvokers) {
      LynxRecorderReplayDataModule.jsbInvokers = []
    }
    if (LynxRecorderReplayDataModule.jsbCallbacks) {
      LynxRecorderReplayDataModule.jsbCallbacks = new Map();
    }
    if (this.jsbModule) {
      this.jsbModule.clear()
    }
    this.genericFetcher = undefined;
    this.mediaFetcher = undefined;
    this.templateFetcher = undefined;
    this.localLynxContext = undefined
  }

  private registerJSBModule() {
    let recordData: Map<string, object> = JSON.parse(this.data);
    LynxRecorderReplayDataModule.jsbInvokers = recordData['Invoked Method Data'];
    LynxRecorderReplayDataModule.jsbCallbacks = recordData['Callback'];
    this.jsbModule.set('LynxRecorderReplayDataModule', {
      moduleClass: LynxRecorderReplayDataModule
    });
    this.jsbModuleReady = true;
  }

  private dispatchMessage(that: LynxRecorderView, name: string, params: object) {
    if (name == 'loadTemplate') {
      that.loadTemplate(params as LoadTemplateParams);
    } else if (name == 'loadTemplateBundle') {
      that.loadTemplate(params as LoadTemplateParams);
    } else if (name == 'setGlobalProps') {
      that.setGlobalProps(params as SetGlobalPropsParams);
    } else if (name == 'sendGlobalEvent') {
      that.sendGlobalEvent(params as SendGlobalEventParams);
    } else if (name == 'updateViewPort') {
      // that.updateViewPort(params as UpdateViewPortParams);
    }
    that.changeReadyState();
  }

  private handleActionList() {
    let recordData: Map<string, object> = JSON.parse(this.data);
    let actions: Map<string, object>[] = recordData['Action List'];
    let startTime: number | undefined;
    for (const action of actions) {
      if (startTime == undefined) {
        startTime = action['RecordMillisecond'] as number;
      }
      let delayTime = action['RecordMillisecond'] as number - startTime;
      let function_name: string = action['Function Name'] as string;
      setTimeout(this.dispatchMessage, delayTime, this, function_name, action['Params']);
    }
  }

  private setGlobalProps(params: SetGlobalPropsParams) {
    if (this.ready) {
      this.localLynxContext?.updateMetaData(new MetaData(undefined, new TemplateData(params.global_props)));
    } else {
      this.globalProps = new TemplateData(params.global_props);
    }
  }

  private sendGlobalEvent(params: SendGlobalEventParams) {
    if (params.arguments.length !== 2) {
      return;
    }
    let name: string = params.arguments[0] as string;
    if (name === 'exposure' || name === 'disexposure') {
      return;
    }
    let args: Object[] = params.arguments[1] as object[];
    this.localLynxContext?.sendGlobalEvent(name, args);
  }

  private updateViewPort(params: UpdateViewPortParams) {
    this.localLynxContext?.updateViewport(params.preferredLayoutWidth, params.layoutWidthMode,
      params.preferredLayoutHeight,
      params.layoutHeightMode);
  }

  /*
   * 1. Function: Remove the invalid character '\u0000' at the end of the base64 result.
   * 2. Reason for the presence of invalid characters:
   *    During the recording phase in LynxRecorder, an incorrect estimation of the buffer length for base64 encoding
   *    led to the presence of invalid characters at the end.
   */
  private removeIllegalChar(source: string) {
    let lastIndex = source.length - 1;
    while (source[lastIndex] === '\u0000') {
      lastIndex--;
    }
    return source.substring(0, lastIndex + 1);
  }

  private loadTemplate(params: LoadTemplateParams) {
    let base64Engine = new util.Base64Helper();
    let rawSource = this.removeIllegalChar(params.source);
    let source = base64Engine.decodeSync(rawSource, util.Type.BASIC);
    this.buffer = source.buffer.slice(0, source.buffer.byteLength);
    this.bufferReady = true;
    this.templateData = new TemplateData(params.templateData);
    this.templateDataReady = true;
  }

  async aboutToAppear() {
    // mock extraTiming here
    this.extraTiming.openTime = new Date().getTime();
    this.extraTiming.containerInitStart = new Date().getTime();
    this.extraTiming.containerInitEnd = new Date().getTime();
    this.extraTiming.prepareTemplateStart = new Date().getTime();
    this.download();
    this.extraTiming.prepareTemplateEnd = new Date().getTime();
  }

  build() {
    Column() {
      Row() {
        Row() {
          Image("resources/rawfile/lynx_recorder.svg").width(28).height(28)
          Text('LynxRecorder').fontSize(20).fontStyle(FontStyle.Italic).fontWeight('800').margin({ bottom: 5, left: 5 })
        }

        Stack()
          .width(24)
          .height(24)
          .borderRadius(12)
          .borderWidth(3)
          .onClick(() => {
            this.ready = false;
            this.handleActionList();
          })
          .stateStyles({
            pressed: {
              .borderColor(Color.Green)
            },
            normal: {
              .borderColor(Color.Black)
            }
          })
          .margin({ right: 15 })
      }
      .width('100%')
      .height(50)
      .padding({ left: 10, right: 10 })
      .justifyContent(FlexAlign.SpaceBetween)
      .borderWidth({ bottom: 1 })

      if (this.ready) {
        LynxView({
          url: this.pageUrl,
          buffer: this.buffer,
          uiRendererCreator: this.uiRendererCreator,
          genericResourceFetcher: this.genericFetcher,
          mediaResourceFetcher: this.mediaFetcher,
          templateResourceFetcher: this.templateFetcher,
          modules: this.jsbModule,
          metaData: new MetaData(this.templateData, this.globalProps),
          onCreate: (context) => {
            this.localLynxContext = context;
            this.localLynxContext.setExtraTiming(this.extraTiming);
          }
        })
      } else {
        Text(`Loading...`).fontSize(30)
      }
    }.size({ width: '100%', height: '100%' })
  }
}

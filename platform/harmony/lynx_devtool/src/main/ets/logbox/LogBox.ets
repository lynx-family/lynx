// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import web_webview from '@ohos.web.webview';
import { GetJSSourceCallback, ILogBox, LLog, LynxError, OnErrorCallback } from '@lynx/lynx';
import { fileIo as fs } from '@kit.CoreFileKit';
import { common } from '@kit.AbilityKit';
import { BusinessError, request } from '@kit.BasicServicesKit';
import { util } from '@kit.ArkTS';
import { BuilderNode, NodeController, UIContext } from '@kit.ArkUI';

interface Event {
  event: string;
  data: LogBoxData;
}

interface LogBoxData {
  log: string;
  namespace: string;
}

interface Result {
  callbackId: number;
  data: string | Object[];
}

interface Message {
  bridgeName: string;
  data: string;
  callbackId: number;
}

interface LogBoxParam {
  text: string,
  logbox: ILogBox,
}

interface FileLoadCallback {
  onSuccess(data: string): void;

  onFailure(reason: string): void;
}

const TAG = 'LogBox';

class NativeBridge {
  errors: LynxError[] = [];
  private webController?: web_webview.WebviewController;
  private dialogController?: CustomDialogController;
  private context?: common.UIAbilityContext;
  private jsSource?: Record<string, string>;

  setWebController(controller: web_webview.WebviewController | undefined) {
    this.webController = controller;
  }

  setAbilityContext(context: common.UIAbilityContext | undefined) {
    this.context = context;
  }

  setDialogController(controller: CustomDialogController | undefined) {
    this.dialogController = controller;
  }

  showErrorMessage(error: LynxError) {
    let eventName = 'receiveNewLog';

    let data: LogBoxData = {
      log: error.getMsg() ?? '',
      namespace: "lynx",
    }
    let event: Event = {
      event: eventName,
      data: data,
    };
    this.sendEvent(event);
  }

  setJsSource(source: Record<string, string>) {
    this.jsSource = source;
  }

  getExceptionStack() {
    // send error message
    for (const error of this.errors) {
      this.showErrorMessage(error);
    }
  }

  postMessage(msg: Message) {
    switch (msg.bridgeName) {
      case 'getExceptionStack':
        this.getExceptionStack();
        this.sendResult(msg.callbackId, []);
        break;
      case 'getCoreJs':
        this.sendResult(msg.callbackId, '');
        break;
      case 'getTemplateJs':
        this.sendResult(msg.callbackId, '');
        break;
      case 'reload':
        this.reload();
        break;
      case 'dismiss':
        this.dismiss();
        break;
      case 'loadErrorParser':
        this.loadErrorParser(msg.callbackId);
        break;
      case 'queryResource':
        try {
          let dataJson: ESObject = JSON.parse(msg.data);
          this.getResource(msg.callbackId, dataJson.name);
        } catch (e) {
          LLog.d(TAG, 'queryResource error: ' + e);
          this.sendResult(msg.callbackId, "");
        }
        break;
      default:
        break;
    }
  }

  private getResource(callbackId: number, name: string) {
    if (!name) {
      this.sendResult(callbackId, "");
    }
    if (name.startsWith("http")) {
      this.download(callbackId, name);
      return;
    }
    if (!this.jsSource) {
      LLog.w(TAG, "the js source cache is null");
      this.sendResult(callbackId, "");
      return;
    }
    let processName = name;
    if (processName.includes("lynx_core.js")) {
      processName = "core.js";
    }
    let src = "";
    for (let entry of Object.entries(this.jsSource as Record<string, string>)) {
      if (entry[0].includes(processName)) {
        this.sendResult(callbackId, entry[1]);
        return;
      }
    }
    // If there is no response, add a fallback to response.
    this.sendResult(callbackId, "");
  }

  private download(callbackId: number, name: string) {
    let logboxDir = this.context?.tempDir + "/" + name;
    try {
      request.downloadFile(this.context, { url: name, filePath: logboxDir }).then((data: request.DownloadTask) => {
        let downloadTask: request.DownloadTask = data;
        let downloadFinishCallback = () => {
          let content = this.readFileContent(logboxDir);
          this.sendResult(callbackId, content);
        };
        downloadTask.on("complete", downloadFinishCallback);
      }).catch((err: BusinessError) => {
        LLog.e(TAG, `Failed to request the download. Code: ${err.code}, message: ${err.message}`);
        this.sendResult(callbackId, "");
      })
    } catch (e) {
      LLog.e(TAG, "downloadFile error: " + e);
      this.sendResult(callbackId, "");
    }
  }

  private readFileContent(filePath: string) {
    let content = fs.readTextSync(filePath)
    return content;
  }

  private sendEvent(event: Event) {
    this.webController?.runJavaScript(`window.logbox.sendEvent(${JSON.stringify(event)});`);
  }

  private sendResult(callbackId: number, data: string | Object[]) {
    const result: Result = {
      callbackId,
      data,
    };
    this.webController?.runJavaScript(`window.logbox.sendResult(${JSON.stringify(result)});`);
  }

  private loadErrorParser(callbackId: number) {
    try {
      let context = getContext(this) as common.UIAbilityContext;
      let resourceManager = context.resourceManager;
      resourceManager.getRawFileContent("logbox/lynx-error-parser.js").then(value => {
        let rawFile = value;
        const textDecoder = new util.TextDecoder("utf-8", { ignoreBOM: true });
        const jsStr = textDecoder.decodeToString(rawFile);
        this.webController?.runJavaScript(jsStr);
        this.sendResult(callbackId, 'true');
      })
    } catch (error) {
      LLog.e(TAG, "load js file failed: " + error.message + ", error code: " + error.code);
      this.sendResult(callbackId, 'false');
    }
  }

  private async loadFile(filename: string): Promise<string> {
    let filesDir = this.context?.filesDir;
    return fs.readText(filesDir + '/' + filename);
  }

  private reload() {
  }

  private dismiss() {
    this.dialogController?.close();
  }
}

@CustomDialog
struct LogBoxDialog {
  controller: CustomDialogController;
  nativeBridge?: NativeBridge;
  webviewController: web_webview.WebviewController = new web_webview.WebviewController();
  url = 'logbox/index.html';
  src = $rawfile(this.url);

  aboutToAppear() {
    this.nativeBridge?.setWebController(this.webviewController);
    this.nativeBridge?.setDialogController(this.controller);
    let context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    this.nativeBridge?.setAbilityContext(context);
  }

  aboutToDisappear() {
    this.nativeBridge?.setWebController(undefined);
    this.nativeBridge?.setDialogController(undefined);
  }

  build() {
    Web({ src: this.src, controller: this.webviewController })
      .javaScriptProxy({
        object: this.nativeBridge,
        name: 'nativeBridge',
        methodList: ['postMessage'],
        controller: this.webviewController
      })
      .onPageEnd(() => {
        this.webviewController.runJavaScript(`
        if (!window.logbox) {
          (function() {
            var id = 0, callbacks = {}, eventListeners = {};
            window.logbox = {
              call: function(bridgeName, callback, data) {
                var thisId = id++;
                callbacks[thisId] = callback;
                nativeBridge.postMessage({
                  bridgeName: bridgeName,
                  data: data ? JSON.stringify(data) : '',
                  callbackId: thisId,
                });
              },
              on: function(event, handler) {
                eventListeners[event] = handler;
              },
              sendResult: function(msg) {
                var callbackId = msg.callbackId;
                if (callbacks[callbackId]) {
                  callbacks[callbackId](msg.data);
                }
              },
              sendEvent: function(msg) {
                if (eventListeners[msg.event]) {
                  eventListeners[msg.event](msg.data);
                }
              },
            };
          })();
          setTimeout(function() {
          document.dispatchEvent(new Event('LogBoxReady'));}, 0);
        }`);
      })
      .height('50%');
  }
}

@Component
export struct LogBoxComponent {
  logBox?: LogBox;
  text?: string;
  private dialogController: CustomDialogController = new CustomDialogController({
    builder: LogBoxDialog({
      nativeBridge: this.logBox?.nativeBridge,
    }),
    alignment: DialogAlignment.Bottom,
    customStyle: true,
    maskColor: 0x33000000,
  });

  build() {

    Row() {
      Text(this.text)
        .maxLines(1)
        .fontColor(Color.White)
        .ellipsisMode(EllipsisMode.END)
        .fontSize(15)
        .width('95%')
        .onClick(() => {
          this.dialogController.open();
        });
      Text('X')
        .alignSelf(ItemAlign.End)
        .fontColor(Color.White)
        .fontSize(15)
        .onClick(() => {
          this.logBox?.cleanError();
        });
    }.margin({
      left: 10,
      right: 10
    })
    .padding({
      top: 3,
      bottom: 3,
      left: 9,
      right: 9
    })
    .backgroundColor(0xE6151515).borderRadius(5)
  }
}

export class LogBox implements ILogBox {
  onError: (msg: string) => void;
  getJsSource: () => Record<string, string>;
  nativeBridge: NativeBridge = new NativeBridge();

  public reportError(err: LynxError) {
    this.nativeBridge.errors.push(err);
    this.onError(err.getSummaryMessage());
  }

  public cleanError() {
    this.nativeBridge.errors.length = 0;
    this.onError('');
  }

  public onLoaded(url: string) {
    let jsSource: Record<string, string> = this.getJsSource();
    this.nativeBridge.setJsSource(jsSource);
  }

  constructor(onError: OnErrorCallback, getJsSource: GetJSSourceCallback) {
    this.onError = onError;
    this.getJsSource = getJsSource;
  }
}

@Builder
function buildLogBoxComponent(params: LogBoxParam) {
  LogBoxComponent({ logBox: params.logbox as LogBox, text: params.text });
}

export class LogBoxNodeController extends NodeController {
  private logBoxNode: BuilderNode<[LogBoxParam]> | null = null;
  private logBox: ILogBox | null = null;
  private text: string;

  constructor(logBox: ILogBox, title: string) {
    super();
    this.logBox = logBox;
    this.text = title;
  }

  makeNode(uiContext: UIContext): FrameNode | null {
    this.logBoxNode = new BuilderNode(uiContext);
    this.logBoxNode.build(wrapBuilder<[LogBoxParam]>(buildLogBoxComponent),
      { text: this.text, logbox: this.logBox } as LogBoxParam);
    return this.logBoxNode.getFrameNode();
  }
}

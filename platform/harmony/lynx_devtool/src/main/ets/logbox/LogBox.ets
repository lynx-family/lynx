// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import web_webview from '@ohos.web.webview';
import { LLog, LynxError, LynxErrorLevel } from '@lynx/lynx';
import { GetJSSourceCallback, ILogBox, OnErrorCallback } from '@lynx/lynx/src/main/ets/devtool_wrapper/ILogBox';
import { fileIo as fs } from '@kit.CoreFileKit';
import { common } from '@kit.AbilityKit';
import { BusinessError, request } from '@kit.BasicServicesKit';
import { util } from '@kit.ArkTS';
import { ComponentContent, UIContext } from '@kit.ArkUI';

interface Event {
  event: string;
  data?: LogBoxData | LogBoxViewInfoData | LogBoxFileData;
}

interface LogBoxData {
  log: string;
  namespace: string;
}

interface LogBoxViewInfoData {
  currentView: number;
  viewsCount: number;
  level: string;
  templateUrl: string;
}

interface LogBoxFileData {
  type: string;
  data: string;
}

interface Result {
  callbackId: number;
  data: string | Object[] | boolean;
}

interface Message {
  bridgeName: string;
  data: string;
  callbackId: number;
}

interface LogBoxDetailParam {
  nativeBridge?: NativeBridge;
  dismissCallback?: () => void;
}

const TAG = 'LogBox';
const LOGBOX_LEVEL_ERROR = 'error';
const LOGBOX_LEVEL_WARN = 'warn';
const LOGBOX_BRIEF_MAX_LENGTH = 50;
const LOGBOX_MAPPINGS_WASM = 'logbox/mappings.wasm';
const LOGBOX_MAPPINGS_WASM_TYPE = 'mappings.wasm';

interface LogBoxNotificationData {
  level: string;
  message: string;
  count: number;
}

interface LogBoxNotificationParam {
  records: LogBoxNotificationData[];
  manager?: LogBoxNotificationManager;
}

class LogBoxSourceRecord {
  count: number = 0;
  message: string = '';
  sequence: number = 0;
}

class LogBoxLevelBucket {
  count: number = 0;
  message: string = '';
  sequence: number = 0;
  logBox?: LogBox;
  currentIndex: number = 0;
  byLogBox: Map<LogBox, LogBoxSourceRecord> = new Map();

  reset() {
    this.count = 0;
    this.message = '';
    this.sequence = 0;
    this.logBox = undefined;
    this.currentIndex = 0;
    this.byLogBox.clear();
  }

  hasRecords(): boolean {
    return this.count > 0 && this.byLogBox.size > 0;
  }

  getLogBoxes(): LogBox[] {
    let logBoxes: LogBox[] = [];
    this.byLogBox.forEach((_record, logBox) => {
      logBoxes.push(logBox);
    });
    return logBoxes;
  }

  getCurrentLogBox(): LogBox | undefined {
    let logBoxes = this.getLogBoxes();
    if (logBoxes.length === 0) {
      this.currentIndex = 0;
      return undefined;
    }
    if (this.currentIndex < 0 || this.currentIndex >= logBoxes.length) {
      this.currentIndex = logBoxes.length - 1;
    }
    return logBoxes[this.currentIndex];
  }

  setCurrentLogBox(logBox: LogBox): boolean {
    let logBoxes = this.getLogBoxes();
    let index = logBoxes.indexOf(logBox);
    if (index < 0) {
      return false;
    }
    this.currentIndex = index;
    return true;
  }

  setCurrentView(viewNumber: number): boolean {
    let logBoxes = this.getLogBoxes();
    if (viewNumber < 1 || viewNumber > logBoxes.length) {
      return false;
    }
    this.currentIndex = viewNumber - 1;
    return true;
  }
}

function normalizeLogBoxLevel(error: LynxError): string {
  let level = error.getLevel();
  if (level === LynxErrorLevel.Warn) {
    return LOGBOX_LEVEL_WARN;
  }
  if (level === LynxErrorLevel.Error || level === LynxErrorLevel.Fatal) {
    return LOGBOX_LEVEL_ERROR;
  }
  return LOGBOX_LEVEL_ERROR;
}

function briefLogBoxMessage(error: LynxError): string {
  let message = error.getSummaryMessage() || error.getMsg() || '';
  message = message.split('\n').join(' ');
  if (message.length > LOGBOX_BRIEF_MAX_LENGTH) {
    return message.substring(0, LOGBOX_BRIEF_MAX_LENGTH) + '...';
  }
  return message;
}

class NativeBridge {
  errors: LynxError[] = [];
  private webController?: web_webview.WebviewController;
  private dismissCallback?: () => void;
  private context?: common.UIAbilityContext;
  private uiContext?: UIContext;
  private jsSource?: Record<string, string>;
  private templateUrl: string = '';
  private manager?: LogBoxNotificationManager;
  private level: string = LOGBOX_LEVEL_ERROR;
  private isWasmLoaded: boolean = false;

  setWebController(controller: web_webview.WebviewController | undefined) {
    if (this.webController !== controller) {
      this.isWasmLoaded = false;
    }
    this.webController = controller;
  }

  setAbilityContext(context: common.UIAbilityContext | undefined) {
    this.context = context;
  }

  setUIContext(uiContext: UIContext | undefined) {
    this.uiContext = uiContext;
  }

  setDismissCallback(callback: (() => void) | undefined) {
    this.dismissCallback = callback;
  }

  setLogBoxManager(manager: LogBoxNotificationManager | undefined, level: string = LOGBOX_LEVEL_ERROR) {
    this.manager = manager;
    this.level = level;
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

  getJsSource(): Record<string, string> {
    return this.jsSource ?? {};
  }

  setTemplateUrl(url: string) {
    this.templateUrl = url;
  }

  getTemplateUrl(): string {
    return this.templateUrl;
  }

  getExceptionStack() {
    if (!this.webController) {
      return;
    }
    if (this.isWasmLoaded) {
      this.sendExceptionStack();
      return;
    }
    this.isWasmLoaded = true;
    this.loadMappingsWasm((loaded: boolean) => {
      if (!loaded) {
        this.isWasmLoaded = false;
      }
      this.sendExceptionStack();
    });
  }

  private sendExceptionStack() {
    if (this.manager) {
      this.manager.requestLogsOfCurrentView(this.level, this);
      return;
    }
    // send error message
    for (const error of this.errors) {
      this.showErrorMessage(error);
    }
  }

  sendViewInfo(currentView: number, viewsCount: number, level: string, templateUrl: string) {
    this.sendEvent({
      event: 'receiveViewInfo',
      data: {
        currentView: currentView,
        viewsCount: viewsCount,
        level: level,
        templateUrl: templateUrl,
      },
    });
  }

  sendReset() {
    this.sendEvent({
      event: 'reset',
    });
  }

  getLogSourceWithFileName(name: string): string {
    if (!this.jsSource) {
      return "";
    }
    let processName = name;
    if (processName.includes("lynx_core.js")) {
      processName = "core.js";
    }
    let value = "";
    let matchLength = 0;
    for (let entry of Object.entries(this.jsSource as Record<string, string>)) {
      let key = entry[0];
      if (processName.endsWith(key) && key.length > matchLength) {
        value = entry[1];
        matchLength = key.length;
      }
    }
    if (!value) {
      for (let entry of Object.entries(this.jsSource as Record<string, string>)) {
        let key = entry[0];
        if (key.includes(processName) && key.length > matchLength) {
          value = entry[1];
          matchLength = key.length;
        }
      }
    }
    return value;
  }

  postMessage(msg: Message) {
    switch (msg.bridgeName) {
      case 'getExceptionStack':
        this.getExceptionStack();
        this.sendResult(msg.callbackId, []);
        break;
      case 'dismiss':
        this.dismiss();
        break;
      case 'changeView':
        try {
          let dataJson: ESObject = JSON.parse(msg.data);
          this.manager?.requestLogsOfViewIndex(Number(dataJson.viewNumber), this.level, this);
        } catch (e) {
          LLog.d(TAG, 'changeView error: ' + e);
        }
        this.sendResult(msg.callbackId, []);
        break;
      case 'deleteLynxview':
        try {
          let dataJson: ESObject = JSON.parse(msg.data);
          this.manager?.removeLogsOfViewIndex(Number(dataJson.viewNumber), this.level, this);
        } catch (e) {
          this.manager?.removeLogsOfCurrentView(this.level, this);
          LLog.d(TAG, 'deleteLynxview error: ' + e);
        }
        this.sendResult(msg.callbackId, []);
        break;
      case 'toast':
        try {
          let dataJson: ESObject = JSON.parse(msg.data);
          this.showToast(dataJson.message as string);
        } catch (e) {
          LLog.d(TAG, 'toast error: ' + e);
        }
        this.sendResult(msg.callbackId, []);
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
      return;
    }
    if (name.startsWith("http")) {
      this.download(callbackId, name);
      return;
    }
    let source = '';
    if (this.manager) {
      source = this.manager.getLogSourceWithFileName(name, this.level);
    } else {
      source = this.getLogSourceWithFileName(name);
    }
    if (source) {
      if (source.startsWith("http")) {
        this.download(callbackId, source);
      } else {
        this.sendResult(callbackId, source);
      }
      return;
    }
    this.sendResult(callbackId, "");
  }

  private showToast(message: string) {
    if (!message) {
      return;
    }
    this.uiContext?.getPromptAction().showToast({
      message: message,
    });
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

  private loadMappingsWasm(callback: (loaded: boolean) => void) {
    try {
      let context: common.UIAbilityContext =
        this.context ?? (getContext(this) as common.UIAbilityContext);
      let resourceManager = context.resourceManager;
      resourceManager.getRawFileContent(LOGBOX_MAPPINGS_WASM).then(value => {
        let base64Helper = new util.Base64Helper();
        let base64Data = base64Helper.encodeToStringSync(value);
        let loadFileResult = this.sendEvent({
          event: 'loadFile',
          data: {
            type: LOGBOX_MAPPINGS_WASM_TYPE,
            data: base64Data,
          },
        });
        if (!loadFileResult) {
          LLog.e(TAG, 'send mappings.wasm skipped, webController is undefined');
          callback(false);
          return;
        }
        loadFileResult.then(() => {
          callback(true);
        }).catch(() => {
          LLog.e(TAG, 'send mappings.wasm failed.');
          callback(false);
        })
      }).catch(() => {
        LLog.e(TAG, 'failed to load mappings.wasm.');
        callback(false);
      })
    } catch (error) {
      LLog.e(TAG, `load mappings.wasm failed. ${error}`);
      callback(false);
    }
  }

  private sendEvent(event: Event): Promise<string> | undefined {
    return this.webController?.runJavaScript(`window.logbox.sendEvent(${JSON.stringify(event)});`);
  }

  private sendResult(callbackId: number, data: string | Object[] | boolean) {
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
        this.sendResult(callbackId, true);
      })
    } catch (error) {
      LLog.e(TAG, "load js file failed: " + error.message + ", error code: " + error.code);
      this.sendResult(callbackId, false);
    }
  }

  private dismiss() {
    this.dismissCallback?.();
  }
}

@Component
struct LogBoxDialog {
  nativeBridge?: NativeBridge;
  dismissCallback?: () => void;
  webviewController: web_webview.WebviewController = new web_webview.WebviewController();
  url = 'logbox/index.html';
  src = $rawfile(this.url);

  aboutToAppear() {
    this.nativeBridge?.setWebController(this.webviewController);
    this.nativeBridge?.setDismissCallback(this.dismissCallback);
    this.nativeBridge?.setUIContext(this.getUIContext());
    let context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    this.nativeBridge?.setAbilityContext(context);
  }

  aboutToDisappear() {
    this.nativeBridge?.setWebController(undefined);
    this.nativeBridge?.setDismissCallback(undefined);
    this.nativeBridge?.setUIContext(undefined);
    this.nativeBridge?.setAbilityContext(undefined);
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
      .width('100%')
      .height('50%');
  }
}

@Builder
function buildLogBoxDialog(params: LogBoxDetailParam) {
  LogBoxDialog({
    nativeBridge: params.nativeBridge,
    dismissCallback: params.dismissCallback,
  });
}

@Component
struct LogBoxNotificationItem {
  @Prop
  record: LogBoxNotificationData = {
    level: LOGBOX_LEVEL_ERROR,
    message: '',
    count: 0,
  };
  manager?: LogBoxNotificationManager;

  private getCountText(): string {
    let count = this.record.count.toString();
    if (count.length > 2) {
      return count.substring(0, 1) + '...';
    }
    return count;
  }

  private getBadgeColor(): number {
    return this.record.level === LOGBOX_LEVEL_WARN ? 0xFFEFBD36 : 0xFFE44369;
  }

  build() {
    Row() {
      Row() {
        Text(this.getCountText())
          .width(27)
          .height(27)
          .borderRadius(14)
          .borderWidth(2)
          .borderColor(Color.White)
          .backgroundColor(this.getBadgeColor())
          .fontColor(Color.White)
          .fontSize(14)
          .textAlign(TextAlign.Center)
        Column()
          .width(1)
          .height(27)
          .backgroundColor(0xFF5C5D5D)
          .margin({
            left: 9,
            right: 9,
          })
        Text(this.record.message)
          .maxLines(1)
          .fontColor(Color.White)
          .ellipsisMode(EllipsisMode.END)
          .fontSize(15)
          .layoutWeight(1)
      }
      .height('100%')
      .layoutWeight(1)
      .onClick(() => {
        this.manager?.openLevel(this.record.level);
      })

      Stack() {
        Stack() {
          Image($r('app.media.notification_cancel'))
            .width(12)
            .height(12)
            .objectFit(ImageFit.Contain)
        }
        .width(20)
        .height(20)
        .borderRadius(10)
        .backgroundColor(0xFF717272)
      }
      .width(45)
      .height('100%')
      .onClick(() => {
        this.manager?.clearLevel(this.record.level);
      })
    }
    .width('100%')
    .height(45)
    .padding({
      left: 9,
    })
    .margin({
      left: 5,
      right: 5,
      bottom: 5,
    })
    .backgroundColor(0xE6151515)
    .borderRadius(8)
  }
}

@Component
struct LogBoxNotificationDialog {
  @Prop
  records: LogBoxNotificationData[] = [];
  manager?: LogBoxNotificationManager;

  build() {
    Column() {
      ForEach(this.records, (record: LogBoxNotificationData) => {
        LogBoxNotificationItem({
          record: record,
          manager: this.manager,
        })
      }, (record: LogBoxNotificationData) => `${record.level}-${record.count}`)
    }
    .width('100%')
  }
}

@Builder
function buildLogBoxNotificationDialog(params: LogBoxNotificationParam) {
  LogBoxNotificationDialog({
    records: params.records,
    manager: params.manager,
  });
}

class LogBoxContentHolder<T extends Object> {
  uiContext: UIContext;
  content: ComponentContent<T>;
  nativeBridge?: NativeBridge;

  constructor(uiContext: UIContext, content: ComponentContent<T>, nativeBridge?: NativeBridge) {
    this.uiContext = uiContext;
    this.content = content;
    this.nativeBridge = nativeBridge;
  }
}

class LogBoxNotificationManager {
  private uiContext: UIContext;
  private windowName?: string;
  private hostContext?: object;
  private logBoxes: LogBox[] = [];
  private errorBucket: LogBoxLevelBucket = new LogBoxLevelBucket();
  private warnBucket: LogBoxLevelBucket = new LogBoxLevelBucket();
  private nextSequence: number = 0;
  private notificationContent?: LogBoxContentHolder<LogBoxNotificationParam>;
  private isNotificationOpen: boolean = false;
  private detailContent?: LogBoxContentHolder<LogBoxDetailParam>;
  private detailLogBox?: LogBox;
  private detailBridge?: NativeBridge;
  private detailLevel?: string;

  constructor(uiContext: UIContext) {
    this.uiContext = uiContext;
    this.windowName = uiContext.getWindowName();
    this.hostContext = uiContext.getHostContext();
  }

  matches(uiContext: UIContext): boolean {
    let windowName = uiContext.getWindowName();
    if (this.windowName && windowName) {
      return this.windowName === windowName;
    }
    let hostContext = uiContext.getHostContext();
    if (this.hostContext && hostContext) {
      return this.hostContext === hostContext;
    }
    return this.uiContext === uiContext;
  }

  isUnused(): boolean {
    return this.logBoxes.length === 0 && !this.hasRecords();
  }

  hasLogBox(logBox: LogBox): boolean {
    return this.logBoxes.indexOf(logBox) >= 0 || this.errorBucket.byLogBox.has(logBox) ||
      this.warnBucket.byLogBox.has(logBox);
  }

  registerLogBox(logBox: LogBox, uiContext: UIContext) {
    this.uiContext = uiContext;
    this.windowName = uiContext.getWindowName() ?? this.windowName;
    let hostContext = uiContext.getHostContext();
    this.hostContext = hostContext ?? this.hostContext;
    if (this.logBoxes.indexOf(logBox) < 0) {
      this.logBoxes.push(logBox);
    }
    this.reloadLogBox(logBox);
    this.syncNotifications();
  }

  unregisterLogBox(logBox: LogBox) {
    let index = this.logBoxes.indexOf(logBox);
    if (index >= 0) {
      this.logBoxes.splice(index, 1);
    }
    logBox.detach();
    this.syncNotifications();
  }

  addLog(logBox: LogBox, error: LynxError) {
    let level = normalizeLogBoxLevel(error);
    let bucket = this.getBucket(level);
    let isDetailShowingLevel = !!this.detailContent && this.detailLevel === level;
    let isCurrentSource = bucket.getCurrentLogBox() === logBox;
    let isNewSource = !bucket.byLogBox.has(logBox);
    this.addLogToBucket(bucket, logBox, briefLogBoxMessage(error), !isDetailShowingLevel);
    if (isDetailShowingLevel) {
      if (isCurrentSource) {
        this.detailBridge?.showErrorMessage(error);
      }
      if (isNewSource) {
        let currentLogBox = bucket.getCurrentLogBox();
        this.detailBridge?.sendViewInfo(bucket.currentIndex + 1, bucket.getLogBoxes().length, level,
          currentLogBox?.nativeBridge.getTemplateUrl() ?? '');
      }
    }
    this.syncNotifications();
  }

  clearLogBox(logBox: LogBox) {
    this.removeLogBox(logBox);
    if (this.detailLogBox === logBox) {
      this.closeDetailDialog();
    }
    this.syncNotifications();
  }

  openLevel(level: string) {
    let bucket = this.getBucket(level);
    if (!bucket.logBox) {
      return;
    }
    bucket.setCurrentLogBox(bucket.logBox);
    this.openDetailDialog(level, bucket.logBox);
  }

  clearLevel(level: string) {
    let bucket = this.getBucket(level);
    bucket.byLogBox.forEach((_record, logBox) => {
      logBox.clearErrorsForNotificationLevel(level);
    });
    bucket.reset();
    if (this.detailLevel === level) {
      this.closeDetailDialog();
    }
    this.syncNotifications();
    LogBoxNotificationOwner.cleanupUnusedManagers();
  }

  dispose() {
    this.closeNotificationDialog();
    this.closeDetailDialog();
    this.logBoxes.length = 0;
    this.errorBucket.reset();
    this.warnBucket.reset();
  }

  requestLogsOfCurrentView(level: string, bridge: NativeBridge) {
    let bucket = this.getBucket(level);
    let logBox = bucket.getCurrentLogBox();
    let logBoxes = bucket.getLogBoxes();
    bridge.sendReset();
    if (!logBox) {
      bridge.sendViewInfo(0, 0, level, '');
      return;
    }
    this.detailLogBox = logBox;
    this.detailLevel = level;
    bridge.sendViewInfo(bucket.currentIndex + 1, logBoxes.length, level, logBox.nativeBridge.getTemplateUrl());
    for (const error of logBox.getErrors()) {
      if (normalizeLogBoxLevel(error) === level) {
        bridge.showErrorMessage(error);
      }
    }
  }

  requestLogsOfViewIndex(viewNumber: number, level: string, bridge: NativeBridge) {
    let bucket = this.getBucket(level);
    bucket.setCurrentView(viewNumber);
    this.requestLogsOfCurrentView(level, bridge);
  }

  removeLogsOfViewIndex(viewNumber: number, level: string, bridge: NativeBridge) {
    let bucket = this.getBucket(level);
    bucket.setCurrentView(viewNumber);
    this.removeLogsOfCurrentView(level, bridge);
  }

  removeLogsOfCurrentView(level: string, bridge: NativeBridge) {
    let bucket = this.getBucket(level);
    let logBox = bucket.getCurrentLogBox();
    if (!logBox) {
      return;
    }
    this.removeLogBoxFromBucket(bucket, logBox);
    logBox.clearErrorsForNotificationLevel(level);
    this.syncNotifications();
    if (bucket.hasRecords()) {
      this.requestLogsOfCurrentView(level, bridge);
    } else {
      this.closeDetailDialog();
    }
    LogBoxNotificationOwner.cleanupUnusedManagers();
  }

  getLogSourceWithFileName(fileName: string, level: string): string {
    let logBox = this.getBucket(level).getCurrentLogBox();
    return logBox?.nativeBridge.getLogSourceWithFileName(fileName) ?? "";
  }

  private reloadLogBox(logBox: LogBox) {
    this.removeLogBox(logBox);
    for (const error of logBox.getErrors()) {
      let level = normalizeLogBoxLevel(error);
      this.addLogToBucket(this.getBucket(level), logBox, briefLogBoxMessage(error));
    }
  }

  private addLogToBucket(bucket: LogBoxLevelBucket, logBox: LogBox, message: string, selectCurrent: boolean = true) {
    let record = bucket.byLogBox.get(logBox);
    if (!record) {
      record = new LogBoxSourceRecord();
      bucket.byLogBox.set(logBox, record);
    }
    record.count++;
    record.message = message;
    record.sequence = ++this.nextSequence;
    bucket.count++;
    bucket.message = message;
    bucket.sequence = record.sequence;
    bucket.logBox = logBox;
    if (selectCurrent) {
      bucket.setCurrentLogBox(logBox);
    }
  }

  private removeLogBox(logBox: LogBox) {
    this.removeLogBoxFromBucket(this.errorBucket, logBox);
    this.removeLogBoxFromBucket(this.warnBucket, logBox);
  }

  private removeLogBoxFromBucket(bucket: LogBoxLevelBucket, logBox: LogBox) {
    if (!bucket.byLogBox.has(logBox)) {
      return;
    }
    let previousLogBoxes = bucket.getLogBoxes();
    let removedIndex = previousLogBoxes.indexOf(logBox);
    bucket.byLogBox.delete(logBox);
    this.rebuildBucket(bucket);
    let remainingLogBoxes = bucket.getLogBoxes();
    if (remainingLogBoxes.length === 0) {
      bucket.currentIndex = 0;
    } else if (removedIndex >= 0) {
      bucket.currentIndex = Math.min(removedIndex, remainingLogBoxes.length - 1);
    }
  }

  private rebuildBucket(bucket: LogBoxLevelBucket) {
    bucket.count = 0;
    bucket.message = '';
    bucket.sequence = 0;
    bucket.logBox = undefined;
    bucket.byLogBox.forEach((record, logBox) => {
      bucket.count += record.count;
      if (record.sequence >= bucket.sequence) {
        bucket.message = record.message;
        bucket.sequence = record.sequence;
        bucket.logBox = logBox;
      }
    });
  }

  private hasRecords(): boolean {
    return this.errorBucket.hasRecords() || this.warnBucket.hasRecords();
  }

  private getBucket(level: string): LogBoxLevelBucket {
    return level === LOGBOX_LEVEL_WARN ? this.warnBucket : this.errorBucket;
  }

  private buildRecords(): LogBoxNotificationData[] {
    let records: LogBoxNotificationData[] = [];
    if (this.errorBucket.count > 0) {
      records.push({
        level: LOGBOX_LEVEL_ERROR,
        message: this.errorBucket.message,
        count: this.errorBucket.count,
      });
    }
    if (this.warnBucket.count > 0) {
      records.push({
        level: LOGBOX_LEVEL_WARN,
        message: this.warnBucket.message,
        count: this.warnBucket.count,
      });
    }
    return records;
  }

  private syncNotifications() {
    let records = this.buildRecords();
    if (records.length === 0) {
      this.closeNotificationDialog();
      return;
    }
    this.openNotificationDialog(records);
  }

  private openNotificationDialog(records: LogBoxNotificationData[]) {
    let params: LogBoxNotificationParam = {
      records: records,
      manager: this,
    };
    if (this.notificationContent) {
      this.notificationContent.content.update(params);
      if (!this.isNotificationOpen) {
        this.openCurrentNotificationContent(this.notificationContent);
      }
      return;
    }
    let content = new ComponentContent(this.uiContext, wrapBuilder<[LogBoxNotificationParam]>(buildLogBoxNotificationDialog),
      params);
    this.notificationContent = new LogBoxContentHolder<LogBoxNotificationParam>(this.uiContext, content);
    this.openCurrentNotificationContent(this.notificationContent);
  }

  private openCurrentNotificationContent(holder: LogBoxContentHolder<LogBoxNotificationParam>) {
    if (this.notificationContent !== holder) {
      holder.content.dispose();
      return;
    }
    this.isNotificationOpen = true;
    holder.uiContext.getPromptAction().openCustomDialog(holder.content, {
      alignment: DialogAlignment.Bottom,
      maskColor: 0x00000000,
      autoCancel: false,
      isModal: false,
      focusable: false,
      onDidDisappear: () => {
        if (this.notificationContent === holder) {
          this.isNotificationOpen = false;
        }
      },
    }).catch((err: BusinessError) => {
      LLog.e(TAG, `Failed to open LogBox notification. Code: ${err.code}, message: ${err.message}`);
      if (this.notificationContent === holder) {
        this.isNotificationOpen = false;
      }
    });
  }

  private closeNotificationDialog() {
    if (!this.notificationContent) {
      this.isNotificationOpen = false;
      return;
    }
    let holder = this.notificationContent;
    this.notificationContent = undefined;
    this.isNotificationOpen = false;
    holder.uiContext.getPromptAction().closeCustomDialog(holder.content).then(() => {
      holder.content.dispose();
    }).catch((err: BusinessError) => {
      LLog.e(TAG, `Failed to close LogBox notification. Code: ${err.code}, message: ${err.message}`);
      holder.content.dispose();
    });
  }

  private openDetailDialog(level: string, logBox: LogBox) {
    if (this.detailContent) {
      this.closeDetailDialog();
    }
    logBox.nativeBridge.setLogBoxManager(this, level);
    let params: LogBoxDetailParam = {
      nativeBridge: logBox.nativeBridge,
      dismissCallback: () => {
        this.closeDetailDialog();
      },
    };
    let content = new ComponentContent(this.uiContext, wrapBuilder<[LogBoxDetailParam]>(buildLogBoxDialog), params);
    let holder = new LogBoxContentHolder<LogBoxDetailParam>(this.uiContext, content, logBox.nativeBridge);
    this.detailContent = holder;
    this.detailLogBox = logBox;
    this.detailBridge = logBox.nativeBridge;
    this.detailLevel = level;
    holder.uiContext.getPromptAction().openCustomDialog(holder.content, {
      alignment: DialogAlignment.Bottom,
      maskColor: 0x33000000,
      onDidDisappear: () => {
        this.releaseDetailContent(holder);
      },
    }).catch((err: BusinessError) => {
      LLog.e(TAG, `Failed to open LogBox detail. Code: ${err.code}, message: ${err.message}`);
      this.releaseDetailContent(holder);
    });
  }

  private closeDetailDialog() {
    if (!this.detailContent) {
      this.detailLogBox = undefined;
      this.detailBridge = undefined;
      this.detailLevel = undefined;
      return;
    }
    let holder = this.detailContent;
    holder.uiContext.getPromptAction().closeCustomDialog(holder.content).catch((err: BusinessError) => {
      LLog.e(TAG, `Failed to close LogBox detail. Code: ${err.code}, message: ${err.message}`);
      this.releaseDetailContent(holder);
    });
  }

  private releaseDetailContent(holder: LogBoxContentHolder<LogBoxDetailParam>) {
    if (this.detailContent !== holder) {
      if (this.detailBridge !== holder.nativeBridge) {
        holder.nativeBridge?.setLogBoxManager(undefined);
      }
      holder.content.dispose();
      return;
    }
    holder.nativeBridge?.setLogBoxManager(undefined);
    this.detailContent = undefined;
    this.detailLogBox = undefined;
    this.detailBridge = undefined;
    this.detailLevel = undefined;
    holder.content.dispose();
  }
}

export class LogBoxNotificationOwner {
  private static instance?: LogBoxNotificationOwner;
  private managers: LogBoxNotificationManager[] = [];

  static getInstance(): LogBoxNotificationOwner {
    if (!LogBoxNotificationOwner.instance) {
      LogBoxNotificationOwner.instance = new LogBoxNotificationOwner();
    }
    return LogBoxNotificationOwner.instance;
  }

  static registerLogBox(logBox: ILogBox, uiContext: UIContext) {
    LogBoxNotificationOwner.getInstance().registerLogBoxInternal(logBox as LogBox, uiContext);
  }

  static unregisterLogBox(logBox: ILogBox) {
    LogBoxNotificationOwner.getInstance().unregisterLogBoxInternal(logBox as LogBox);
  }

  static reportError(logBox: LogBox, error: LynxError) {
    let manager = LogBoxNotificationOwner.getInstance().findManagerByLogBox(logBox);
    manager?.addLog(logBox, error);
  }

  static clearLogBox(logBox: LogBox) {
    let owner = LogBoxNotificationOwner.getInstance();
    let manager = owner.findManagerByLogBox(logBox);
    manager?.clearLogBox(logBox);
    owner.removeUnusedManagers();
  }

  static cleanupUnusedManagers() {
    LogBoxNotificationOwner.getInstance().removeUnusedManagers();
  }

  private registerLogBoxInternal(logBox: LogBox, uiContext: UIContext) {
    let manager = this.getOrCreateManager(uiContext);
    manager.registerLogBox(logBox, uiContext);
  }

  private unregisterLogBoxInternal(logBox: LogBox) {
    let manager = this.findManagerByLogBox(logBox);
    if (!manager) {
      return;
    }
    manager.unregisterLogBox(logBox);
    this.removeUnusedManagers();
  }

  private getOrCreateManager(uiContext: UIContext): LogBoxNotificationManager {
    for (const manager of this.managers) {
      if (manager.matches(uiContext)) {
        return manager;
      }
    }
    let manager = new LogBoxNotificationManager(uiContext);
    this.managers.push(manager);
    return manager;
  }

  private findManagerByLogBox(logBox: LogBox): LogBoxNotificationManager | undefined {
    for (const manager of this.managers) {
      if (manager.hasLogBox(logBox)) {
        return manager;
      }
    }
    return undefined;
  }

  private removeUnusedManagers() {
    let activeManagers: LogBoxNotificationManager[] = [];
    for (const manager of this.managers) {
      if (manager.isUnused()) {
        manager.dispose();
      } else {
        activeManagers.push(manager);
      }
    }
    this.managers = activeManagers;
  }
}

export class LogBox implements ILogBox {
  onError: (msg: string) => void;
  getJsSource: () => Record<string, string>;
  nativeBridge: NativeBridge = new NativeBridge();

  public reportError(err: LynxError) {
    this.nativeBridge.errors.push(err);
    LogBoxNotificationOwner.reportError(this, err);
    this.onError(briefLogBoxMessage(err));
  }

  public cleanError() {
    this.nativeBridge.errors.length = 0;
    LogBoxNotificationOwner.clearLogBox(this);
    this.onError('');
  }

  public getErrors(): LynxError[] {
    return this.nativeBridge.errors;
  }

  public clearErrorsForNotificationLevel(level: string) {
    this.nativeBridge.errors = this.nativeBridge.errors.filter((error) => normalizeLogBoxLevel(error) !== level);
    if (this.nativeBridge.errors.length === 0) {
      this.onError('');
    }
  }

  public onLoaded(url: string) {
    let jsSource: Record<string, string> = this.getJsSource();
    this.nativeBridge.setJsSource(jsSource);
    this.nativeBridge.setTemplateUrl(url);
  }

  public detach() {
    let jsSource = this.nativeBridge.getJsSource();
    this.onError = (_msg: string) => {
    };
    this.getJsSource = (): Record<string, string> => {
      return jsSource;
    };
  }

  constructor(onError: OnErrorCallback, getJsSource: GetJSSourceCallback) {
    this.onError = onError;
    this.getJsSource = getJsSource;
  }
}

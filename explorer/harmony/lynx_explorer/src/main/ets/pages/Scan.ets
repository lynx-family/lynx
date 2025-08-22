// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import router from '@ohos.router';
import Constants from '../common/constants';
import common from '@ohos.app.ability.common';
import { AsyncCallback, BusinessError } from '@ohos.base';
import { abilityAccessCtrl } from '@kit.AbilityKit';
import { customScan, scanBarcode, scanCore } from '@kit.ScanKit';
import { display, promptAction } from '@kit.ArkUI';
import { LLog, LynxDebugger, LynxEnv } from '@lynx/lynx';
import deviceInfo from '@ohos.deviceInfo';
import { LynxRecorderEnv } from '@lynx/lynx_devtool';

const TAG = 'ScanPage'

const screenMetrics = display.getDefaultDisplaySync();
const RATIO: number = screenMetrics.height / screenMetrics.width;

const needScanQR: boolean = true;

interface LynxParam {
  url: string;
}

@Entry
@Component
struct Scan {
  @State hasPermission: boolean = false
  @State isStopCamera: boolean = false
  @State surfaceId: string = ''
  @State cameraHeight: number = 800
  @State cameraWidth: number = 450
  @State cameraOffsetX: number = 0
  @State cameraOffsetY: number = 0
  @State url: string = '';
  private mXComponentController: XComponentController = new XComponentController()
  private callback: AsyncCallback<Array<scanBarcode.ScanResult>> = async (error: BusinessError, result:
    Array<scanBarcode.ScanResult>) => {
    if (error) {
      LLog.e(TAG, "ScanResult error " + JSON.stringify(error));
      return;
    }
    this.showScanResult(result);
  }

  async requestCameraPermission() {
    let grantStatus = await this.reqPermissionsFromUser()
    for (let i = 0; i < grantStatus.length; i++) {
      if (grantStatus[i] === 0) {
        this.hasPermission = true;
      }
    }
  }

  async reqPermissionsFromUser(): Promise<number[]> {
    let context = getContext() as common.UIAbilityContext;
    let atManager = abilityAccessCtrl.createAtManager();
    let grantStatus = await atManager.requestPermissionsFromUser(context, ['ohos.permission.CAMERA']);
    return grantStatus.authResults;
  }

  async showScanResult(result: Array<scanBarcode.ScanResult>) {
    if (result.length > 0) {
      try {
        this.isStopCamera = true;
        customScan.stop((error: BusinessError) => {
          if (error) {
            this.isStopCamera = false;
            LLog.e(TAG, "stop error " + JSON.stringify(error));
            return;
          }
        })
      } catch (error) {
        LLog.e(TAG, "stop error " + JSON.stringify(error));
      }
      this.url = result[0].originalValue;
      setTimeout(() => {
        if (this.url.length !== 0) {
          LLog.d(TAG, 'open lynx, ' + this.url)
          if (this.url.startsWith(Constants.DEVTOOL)) {
            LynxEnv.setTracingDirPath(getContext(this).filesDir);
            let options = new Map<string, string>();
            options.set('App', 'LynxPlayground');
            options.set('AppVersion', '0.0.1');
            options.set('deviceModel', deviceInfo.productModel);
            options.set('osVersion', deviceInfo.distributionOSVersion);
            options.set('sdkVersion', String(deviceInfo.sdkApiVersion));
            LynxDebugger.connectDevtools(this.url, options);
            promptAction.showToast({
              message: 'Devtool connected'
            });
            router.back();
          } else if (this.url.startsWith(LynxRecorderEnv.RECORDER_URL_PREFIX)) {
            LLog.d(TAG, 'open LynxRecorder, ' + this.url)
            router.pushUrl({
              url: Constants.LYNX_RECORDER_URL,
              params: {
                url: this.url,
              }
            }).catch((error: Error) => {
              LLog.d(TAG, 'open LynxRecorder error, ' + JSON.stringify(error));
            });
          } else {
            router.pushUrl({
              url: Constants.LYNX_URL,
              params: {
                url: this.url,
              }
            }).catch((error: Error) => {
              LLog.d(TAG, 'open lynx error, ' + JSON.stringify(error));
            });
          }
        }
      }, 500);
    }
  }

  async onPageShow(): Promise<void> {
    if (needScanQR) {
      await this.requestCameraPermission();
      let options: scanBarcode.ScanOptions = {
        scanTypes: [scanCore.ScanType.ALL],
        enableMultiMode: true,
        enableAlbum: true
      }
      try {
        customScan.init(options);
      } catch (error) {
        LLog.e(TAG, "customScan.init error " + JSON.stringify(error));
      }
    }
  }

  onPageHide(): void {
    if (needScanQR) {
      this.hasPermission = false;
      if (!this.isStopCamera) {
        try {
          customScan.stop((error: BusinessError) => {
            if (error) {
              LLog.e(TAG, "customScan.stop error " + JSON.stringify(error));
              return;
            }
          })
        } catch (error) {
          LLog.e(TAG, "customScan.stop error " + JSON.stringify(error));
        }
      }
      try {
        customScan.release((error: BusinessError) => {
          if (error) {
            LLog.e(TAG, "customScan.release error " + JSON.stringify(error));
            return;
          }
        })
      } catch (error) {
        LLog.e(TAG, "customScan.release error " + JSON.stringify(error));
      }
    }
  }

  @Builder
  TopTool() {
    Column() {
      Flex({ direction: FlexDirection.Row, justifyContent: FlexAlign.SpaceBetween, alignItems: ItemAlign.Center }) {
        Text('Cancel')
          .fontSize(15)
          .fontColor('#FFFFFF')
          .onClick(async () => {
            router.back();
          })
      }.padding({ left: 24, right: 24, top: 24 })

      Column() {
        Text("Scan QR Code").fontSize(15).fontColor("#FFFFFF")
      }.margin({ left: 24, right: 24, top: 24 })
    }
    .height(146)
    .width('100%')
  }

  calcArea(width: number, height: number): void {
    if (width > 0 && height > 0) {
      let maxLen: number = Math.max(width, height);
      let minLen: number = Math.min(width, height);
      if (width > height) {
        if (width / height > RATIO) {
          this.cameraWidth = maxLen;
          this.cameraHeight = maxLen / RATIO;
          this.cameraOffsetX = 0;
          this.cameraOffsetY = (minLen - this.cameraHeight) / 2;
        } else {
          this.cameraHeight = minLen;
          this.cameraWidth = minLen * RATIO;
          this.cameraOffsetX = (maxLen - this.cameraWidth) / 2;
          this.cameraOffsetY = 0;
        }
      } else {
        if (height / width > RATIO) {
          this.cameraHeight = maxLen;
          this.cameraWidth = maxLen / RATIO;
          this.cameraOffsetX = (minLen - this.cameraWidth) / 2;
          this.cameraOffsetY = 0;
        } else {
          this.cameraWidth = minLen;
          this.cameraHeight = minLen * RATIO;
          this.cameraOffsetX = 0;
          this.cameraOffsetY = (maxLen - this.cameraHeight) / 2;
        }
      }
    }
  }

  @Builder
  noScanView() {
    Column() {
      Text('ScanPage').width(Constants.FULL_WIDTH).fontSize(50).textAlign(TextAlign.Center).margin(20)
      TextInput({ placeholder: 'hdc file send url.txt /storage/media/100/local/files/Download' }).margin(20)
        .width(Constants.FULL_WIDTH)
        .borderWidth(1)
        .onChange((value: string) => {
          this.url = value;
        })
      Button('Go').width(Constants.FULL_WIDTH).margin(20).onClick((event: ClickEvent) => {
        if (this.url.length !== 0) {
          LLog.d(TAG, 'open lynx, ' + this.url)
          router.pushUrl({
            url: Constants.LYNX_URL,
            params: {
              url: this.url,
            }
          }).catch((error: Error) => {
            LLog.d(TAG, 'open lynx error, ' + JSON.stringify(error));
          });
        }
      })
    }
    .size({ width: '100%' })
  }

  @Builder
  scanView() {
    Stack() {
      if (this.hasPermission) {
        Column() {
          XComponent({
            id: 'componentId',
            type: 'surface',
            controller: this.mXComponentController
          })
            .onLoad(async () => {
              this.surfaceId = this.mXComponentController.getXComponentSurfaceId();
              let viewControl: customScan.ViewControl = {
                width: this.cameraWidth,
                height: this.cameraHeight,
                surfaceId: this.surfaceId
              };
              try {
                customScan.start(viewControl, this.callback);
              } catch (error) {
                LLog.e(TAG, "customScan.start error " + JSON.stringify(error));
              }
            })
            .width(this.cameraWidth)
            .height(this.cameraHeight)
            .position({ x: this.cameraOffsetX, y: this.cameraOffsetY })
        }
        .height('100%')
        .width('100%')
        .onAreaChange((oldValue: Area, newValue: Area) => {
          this.calcArea(newValue.width as number, newValue.height as number);
        })
      }

      Column() {
        this.TopTool()
        Column() {
        }
        .layoutWeight(1)
        .width('100%')
      }
    }
    .width('100%')
    .height('100%')
    .padding({
      top: 40
    })
  }

  build() {
    if (needScanQR) {
      this.scanView()
    } else {
      this.noScanView()
    }
  }
}

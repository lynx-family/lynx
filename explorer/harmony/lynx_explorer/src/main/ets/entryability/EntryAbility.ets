// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import url from '@ohos.url';
import { LLog } from '@lynx/lynx';
import { LynxInitProcessor } from '../common/LynxInitProcessor';
import { Configuration } from '@ohos.app.ability.Configuration';
import { ConfigurationConstant } from '@kit.AbilityKit';

const TAG = 'EntryAbility';

export default class EntryAbility extends UIAbility {
  private resolveInitialUrl(want: Want): string {
    const urlFromParams: string = want.parameters?.lynx_initial_url as string ?? '';
    if (urlFromParams) {
      return urlFromParams;
    }

    const uri = want.uri ?? '';
    if (!uri) {
      return '';
    }

    try {
      const urlObj = url.URL.parseURL(uri);
      if (urlObj.protocol === 'lynx:' && urlObj.hostname === 'open') {
        const urlParams = new url.URLParams(urlObj.search);
        return urlParams.get('url') ?? '';
      }
    } catch (e) {
      LLog.e(TAG, `resolveInitialUrl parse uri failed: ${e}`);
    }
    return '';
  }

  private updateInitialUrlStorage(want: Want) {
    const initialUrl = this.resolveInitialUrl(want);
    AppStorage.setOrCreate('lynx_initial_url', initialUrl);
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    this.updateInitialUrlStorage(want);

    let initProcessor: LynxInitProcessor = new LynxInitProcessor(this.context);

    // Initialize LynxService first,
    // as LynxEnv initialization depends on LynxDevToolService from LynxService
    initProcessor.initLynxService();

    initProcessor.initLynxEnv();
    AppStorage.setOrCreate('currentColorMode', this.context.config.colorMode);
  }

  onDestroy(): void {
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // Main window is created, set main page for this ability
    LLog.d(TAG, 'Ability onWindowStageCreate');
    AppStorage.setOrCreate('lynxWindowStage', windowStage);
    windowStage.getMainWindow()
      .then((currentWindow) => {
        currentWindow.setPreferredOrientation(window.Orientation.AUTO_ROTATION_RESTRICTED)

      }).catch((err: Error) => {
      LLog.e(TAG, `getMainWindow error: ${err.message}`);
    })

    windowStage.loadContent('pages/Index', (err, data) => {
      if (err.code) {
        LLog.e(TAG, 'Failed to load the content. Cause: ' + JSON.stringify(err));
        return;
      }
      LLog.d(TAG, 'Succeeded in loading the content. ' + JSON.stringify(data));
    });
  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
    LLog.d(TAG, 'Ability onWindowStageDestroy');
    AppStorage.delete('lynxWindowStage');
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    this.updateInitialUrlStorage(want);
  }

  onForeground(): void {
    // Ability has brought to foreground
    LLog.d(TAG, 'Ability onForeground');
  }

  onBackground(): void {
    // Ability has back to background
    LLog.d(TAG, 'Ability onBackground');
  }

  onConfigurationUpdate(newConfig: Configuration): void {
    let newColorMode = newConfig.colorMode;
    let currentColorMode = AppStorage.get<ConfigurationConstant.ColorMode>('currentColorMode');
    if (newColorMode === currentColorMode) {
      return;
    }
    AppStorage.setOrCreate('currentColorMode', newConfig.colorMode);
  }
};

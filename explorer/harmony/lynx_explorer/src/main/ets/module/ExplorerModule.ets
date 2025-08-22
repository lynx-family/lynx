// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LLog, LynxModule } from '@lynx/lynx';
import { LynxRecorderEnv } from '@lynx/lynx_devtool';
import router from '@ohos.router';
import Constants from '../common/constants';

const TAG = 'ExplorerModule'

export class ExplorerModule extends LynxModule {
  public openScan() {
    console.log('ExplorerModule', 'openScan');
    router.pushUrl({
      url: Constants.SCAN_URL,
    }).catch((error: Error) => {
      LLog.d(TAG, 'open scan error, ' + JSON.stringify(error));
    });
  }

  public openSchema(url: string) {
    console.log('ExplorerModule', 'openSchema', url);
    let pageUrl = Constants.LYNX_URL;
    if (url.startsWith(LynxRecorderEnv.RECORDER_URL_PREFIX)) {
      pageUrl = Constants.LYNX_RECORDER_URL
    }
    router.pushUrl({
      url: pageUrl,
      params: {
        url: url,
      }
    }).then(() => {
      LLog.d(TAG, 'openSchema succeed');
    }).catch((error: Error) => {
      LLog.d(TAG, 'openSchema error, ' + JSON.stringify(error));
    });
  }
}

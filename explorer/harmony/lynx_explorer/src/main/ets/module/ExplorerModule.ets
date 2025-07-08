// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LLog, LynxModule } from '@lynx/lynx';
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
    if (url.startsWith(Constants.TEST_BENCH)) {
      pageUrl = Constants.TESTBENCH_URL
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

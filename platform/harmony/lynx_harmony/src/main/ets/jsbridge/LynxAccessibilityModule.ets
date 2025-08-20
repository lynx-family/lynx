// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import accessibility from '@ohos.accessibility';
import { bundleManager, common } from '@kit.AbilityKit';
import { LynxModule } from '../jsbridge/LynxModule';
import { LynxContext } from '../tasm/LynxContext';
import { LLog } from '../base/LLog';

const TAG = 'LynxAccessibilityModule';

interface ContentData {
  content: string;
}

export class LynxAccessibilityModule extends LynxModule {
  private bundleName: string;

  constructor(context: LynxContext, param?: Object) {
    super(context, param);
    let appContext = getContext(this) as common.UIAbilityContext;
    this.bundleName = appContext.abilityInfo.bundleName;
  }

  accessibilityAnnounce(content: ContentData, callback: (res: string) => void): void {
    let errMsg: string = '';
    if (content === undefined || content.content === undefined || content.content.length === 0) {
      errMsg = 'Params error: no content found';
    } else {
      let eventInfo: accessibility.EventInfo = ({
        type: 'announceForAccessibility',
        bundleName: this.bundleName,
        triggerAction: 'common',
        textAnnouncedForAccessibility: content.content,
      });
      accessibility.sendAccessibilityEvent(eventInfo).catch((error: Error) => {
        LLog.e(TAG, 'sendAccessibilityEvent failed, ' + JSON.stringify(error));
      });
      errMsg = 'Success';
    }
    if (callback) {
      callback(errMsg);
    }
  }
}

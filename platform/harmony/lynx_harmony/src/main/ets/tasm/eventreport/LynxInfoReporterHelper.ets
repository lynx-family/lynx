// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LLog } from '../../base/LLog'
import { LynxReportEventPropValue } from './ILynxEventReportObserver'
import { LynxEventReporter } from './LynxEventReporter'
import lynx from 'liblynx.so'

const TAG = 'LynxInfoReporterHelper'

const KEY_TEMPLATE_EVENT = 'lynxsdk_template_event'
const KEY_LAST_LYNX_URL = 'last_lynx_url'
const KEY_ASYNC_COMPONENT_URL = 'last_lynx_async_component_url'
const UNKNOWN_INSTANCE_ID = -1

export enum LynxTemplateType {
  PAGE = 0,
  ASYNC_COMPONENT = 1,
}

/**
 * Help to to report template loading info by LynxEventReporter
 */
class LynxInfoReporterHelperImpl {
  public initialize() {
    lynx.LynxInfoReporterHelper.nativeRegister(LynxInfoReporterHelper)
  }

  public isLynxTemplateEvent(eventKey: string) {
    return eventKey === KEY_TEMPLATE_EVENT
  }

  public reportLynxTemplateEvent(type: LynxTemplateType, value: string) {
    let builder = new Map<string, LynxReportEventPropValue>();
    switch (type) {
      case LynxTemplateType.PAGE:
        builder.set(KEY_LAST_LYNX_URL, value)
        break
      case LynxTemplateType.ASYNC_COMPONENT:
        builder.set(KEY_ASYNC_COMPONENT_URL, value)
        break
    }
    LLog.i(TAG, `reportLynxTemplateEvent with type: ${type}, value: ${value}`)
    LynxEventReporter.onEventSync(KEY_TEMPLATE_EVENT, UNKNOWN_INSTANCE_ID, builder)
  }
}

export const LynxInfoReporterHelper = new LynxInfoReporterHelperImpl()
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export type LynxReportEventPropValue = string | number | boolean;

export interface ILynxEventReportObserver {
  /**
   * Callback method to listen and handle Lynx reporting events.
   * Called in report thread.
   * @param eventName name of event.
   * @param instanceId the unique id of lynx template instance.
   * @param props all properties of event. This's an unmodifiable Map, modification operation will
   *     throw an `UnsupportedOperationException` exception.
   * @param extraData parameters set by method `LynxView.putParamsForReportingEvents()` of report
   *     event. This's an unmodifiable Map, modification operation will throw an
   * `UnsupportedOperationException` exception.
   */
  onReportEvent(eventName: string, instanceId: number, props?: ReadonlyMap<string, LynxReportEventPropValue>,
    extraData?: ReadonlyMap<string, LynxReportEventPropValue>): void;

  /**
   * @Optional
   * Callback method to listen and handle Lynx reporting events synchronously.
   * Called in any thread, in most case, in UI thread
   * Normally, lynx will not use this interface to send back information, so you don't have to implement this method.
   * Notice: Do not block in this method since it may be called in the main thread.
   * @param eventName name of event.
   * @param instanceId the unique id of lynx template instance.
   * @param props all properties of event. This is an unmodifiable Map, modification operation will
   *     throw an `UnsupportedOperationException` exception.
   * @param extraData parameters set by method `LynxView.putParamsForReportingEvents()` of report
   *     event. This is an unmodifiable Map, modification operation will throw an
   * `UnsupportedOperationException` exception.
   */
  onReportEventSync?(eventName: string, instanceId: number, props?: ReadonlyMap<string, LynxReportEventPropValue>,
    extraData?: ReadonlyMap<string, LynxReportEventPropValue>): void;
}
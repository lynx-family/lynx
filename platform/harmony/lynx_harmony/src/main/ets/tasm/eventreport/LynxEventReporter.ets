// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LLog } from '../../base/LLog';
import { TraceCategory, TraceEvent } from '../../base/TraceEvent';
import { TraceEventDef } from '../../base/TraceEventDef';
import { LynxEnv } from '../LynxEnv';
import { LynxServiceCenter, LynxServiceType } from '../service/LynxServiceCenter';
import { ILynxEventReportObserver, LynxReportEventPropValue } from './ILynxEventReportObserver';

type PropsBuilder = () => Map<string, LynxReportEventPropValue>;
type InstanceId = number;

export class LynxEventReporter {
  private static readonly TAG: string = 'LynxEventReporter';
  public static readonly PROP_NAME_LYNX_SDK_VERSION: string = 'lynx_sdk_version';
  public static readonly PROP_NAME_URL: string = 'url';
  public static readonly PROP_NAME_RELATIVE_PATH: string = 'relative_path';
  public static readonly LYNX_SDK_OPEN_PAGE_EVENT: string = 'lynxsdk_open_page';
  private mAllGenericInfos: Map<InstanceId, Map<string, LynxReportEventPropValue>> = new Map();
  private mAllExtraParams: Map<InstanceId, Map<string, LynxReportEventPropValue>> = new Map();
  private mObserverList: ILynxEventReportObserver[] = [];
  private eventReporterService?: ILynxEventReportObserver;
  public static readonly INSTANCE_ID_UNKNOWN: InstanceId = -1;
  private static instance: LynxEventReporter = new LynxEventReporter();

  static initialize() {
    LynxEventReporter.getInstance();
  }

  public static getInstance(): LynxEventReporter {
    return LynxEventReporter.instance;
  }

  private constructor() {
    lynx.EventReporter.registerJSMethods(this, this.onEventCallByNative, this.updateGenericInfoCallByNative,
      this.clearCacheCallByNative);
  }

  /**
   * Report an event with the specified name and instance ID.
   * @param eventName The name of the event.
   * @param instanceId The ID of the instance.
   * @param props Optional properties of the event.
   */
  public static onEvent(eventName: string, instanceId: InstanceId,
    props?: Map<string, LynxReportEventPropValue>): void {
    LynxEventReporter.onEventInternal(false, eventName, instanceId, props);
  }

  /**
   * Report an event with the specified name and instanceId immediately.
   * This method will block the thread, use it carefully, in most case, should use `onEvent`
   * @param eventName The name of the event.
   * @param instanceId The ID of the instance.
   * @param props Optional properties of the event.
   */
  public static onEventSync(eventName: string, instanceId: InstanceId,
    props?: Map<string, LynxReportEventPropValue>): void {
    LynxEventReporter.onEventInternal(true, eventName, instanceId, props);
  }

  private static onEventInternal(sync: boolean, eventName: string, instanceId: InstanceId,
    props?: Map<string, LynxReportEventPropValue>): void {
    if (!eventName) {
      LLog.e(LynxEventReporter.TAG, 'empty event name');
      return;
    }
    let sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = TraceEventDef.EVENT_REPORTER_ON_EVENT;
      const args: Record<string, string> = {
        'instance_id': instanceId + "",
        'eventName': eventName,
        'sync': sync + "",
      }
      TraceEvent.beginSection(TraceCategory.Lynx, sectionName, args);
    }

    if (sync) {
      LynxEventReporter.getInstance().handleEvent(sync, instanceId, eventName, props);
    } else {
      LynxEventReporter.runOnReportThread(() => {
        LynxEventReporter.getInstance().handleEvent(sync, instanceId, eventName, props);
      });
    }
    TraceEvent.endSection(TraceCategory.Lynx, sectionName);
  }

  /**
   * Report an event with the specified name and instance ID using a properties builder.
   * @param eventName The name of the event.
   * @param instanceId The ID of the instance.
   * @param builder The properties builder for the event.
   */
  public static onEventWithBuilder(eventName: string, instanceId: InstanceId, builder: PropsBuilder): void {
    if (!eventName || (instanceId < 0 && !builder)) {
      return;
    }
    let sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = TraceEventDef.EVENT_REPORTER_ON_EVENT;
      const args: Record<string, string> = {
        'instance_id': instanceId + "",
        'eventName': eventName,
      }
      TraceEvent.beginSection(TraceCategory.Lynx, sectionName, args);
    }
    LynxEventReporter.runOnReportThread(() => {
      let props: Map<string, LynxReportEventPropValue> | undefined = undefined;
      if (builder) {
        props = builder();
      }
      if (instanceId < 0 && !props) {
        return;
      }
      LynxEventReporter.getInstance().handleEvent(false, instanceId, eventName, props!);
    });
    TraceEvent.endSection(TraceCategory.Lynx, sectionName);
  }

  /**
   * Update generic information for the specified instance ID.
   * @param key The key of the generic information.
   * @param value The value of the generic information.
   * @param instanceId The ID of the instance.
   */
  public static updateGenericInfo(key: string, value: LynxReportEventPropValue, instanceId: InstanceId): void {
    if (!key || value === null || instanceId < 0) {
      return;
    }
    let sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = TraceEventDef.EVENT_REPORTER_UPDATE_GENERIC_INFO;
      const args: Record<string, string> = {
        'instance_id': instanceId + "",
        'value': value.toString(),
        'key': key
      }
      TraceEvent.beginSection(TraceCategory.Lynx, sectionName, args);
    }
    LynxEventReporter.runOnReportThread(() => {
      LynxEventReporter.getGenericInfoInternal(instanceId).set(key, value);
    });
    TraceEvent.endSection(TraceCategory.Lynx, sectionName);
  }

  /**
   * Add extra parameters for the specified instance ID.
   * @param params The extra parameters to add.
   * @param instanceId The ID of the instance.
   */
  public static putExtraParams(params: ReadonlyMap<string, LynxReportEventPropValue>, instanceId: InstanceId): void {
    if (!params || instanceId < 0) {
      return;
    }
    LynxEventReporter.runOnReportThread(() => {
      const reporter = LynxEventReporter.getInstance();
      let extraParams = reporter.mAllExtraParams.get(instanceId);
      if (!extraParams) {
        extraParams = new Map();
      }
      reporter.setAll(params, extraParams);
    });
  }

  /**
   * Add an external observer.
   * @param observer The observer to add.
   */
  public static addObserver(observer: ILynxEventReportObserver): void {
    LynxEventReporter.getInstance().addObserver(observer);
  }

  private addObserver(observer: ILynxEventReportObserver): void {
    if (observer && !this.mObserverList.includes(observer)) {
      this.mObserverList.push(observer);
    }
  }

  /**
   * Remove an external observer.
   * @param observer The observer to remove.
   */
  public static removeObserver(observer: ILynxEventReportObserver): void {
    LynxEventReporter.getInstance().removeObserver(observer);
  }

  private removeObserver(observer: ILynxEventReportObserver): void {
    if (!observer) {
      return;
    }
    const index = this.mObserverList.indexOf(observer);
    if (index !== -1) {
      this.mObserverList.splice(index, 1);
    }
  }

  /**
   * Run a function on the report thread.
   * @param runnable The function to run.
   */
  public static runOnReportThread(runnable: () => void): void {
    //TODO(limeng.amer): - Call native API to post task to run on the report thread.
    runnable();
  }

  private static getGenericInfoInternal(instanceId: InstanceId): Map<string, LynxReportEventPropValue> {
    const reporter = LynxEventReporter.getInstance();
    let genericInfo = reporter.mAllGenericInfos.get(instanceId);
    if (!genericInfo) {
      genericInfo = new Map();
      genericInfo.set(LynxEventReporter.PROP_NAME_LYNX_SDK_VERSION, LynxEnv.getLynxVersion());
      reporter.mAllGenericInfos.set(instanceId, genericInfo);
    }
    return genericInfo;
  }

  /**
   * Retrieves a generic information map associated with the given instance ID.
   * If no information is found, it initializes the map and records the Lynx SDK version.
   *
   * @param {InstanceId} instanceId - The unique identifier representing an instance for which
   *                                  generic information is queried.
   * @returns {Map<string, LynxReportEventPropValue>} A new Map containing the generic information
   *                                                  associated with the specified instance.
   *                                                  This map includes entries such as the Lynx SDK version.
   */
  public static getGenericInfo(instanceId: InstanceId): Map<string, LynxReportEventPropValue> {
    let genericInfo = LynxEventReporter.getGenericInfoInternal(instanceId);
    // NOTE:
    // Currently, LynxReportEventPropValue is a primitive type (string | number | boolean).
    // Therefore, a shallow copy here is sufficient to avoid any modification of the original Map data.
    // However, if LynxReportEventPropValue expands to include complex types in the future,
    // consider using a deep copy to ensure that internal objects cannot be modified externally.
    return new Map<string, LynxReportEventPropValue>(genericInfo);
  }

  /**
   * Notice: this method may be called in async threads later, please ensure the thread safety
   */
  private handleEvent(sync: boolean, instanceId: InstanceId, eventName: string,
    props?: Map<string, LynxReportEventPropValue>): void {

    let sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = TraceEventDef.EVENT_REPORTER_HANDLE_EVENT;
      const args: Record<string, string> = {
        'instance_id': instanceId + "",
        'eventName': eventName
      }
      TraceEvent.beginSection(TraceCategory.Lynx, sectionName, args);
    }
    if (!eventName) {
      console.error(`${LynxEventReporter.TAG}: event name is null.`);
      TraceEvent.endSection(TraceCategory.Lynx, sectionName);
      return;
    }
    const propsData: Map<string, LynxReportEventPropValue> = new Map();
    const genericInfo = this.mAllGenericInfos.get(instanceId);
    if (genericInfo) {
      this.setAll(genericInfo, propsData);
    }
    if (props) {
      this.setAll(props, propsData);
    }
    const extraParams = this.mAllExtraParams.get(instanceId);
    let extraData: Map<string, LynxReportEventPropValue> | undefined = undefined;
    if (extraParams) {
      extraData = new Map<string, LynxReportEventPropValue>(extraParams);
    }
    this.setupEventReporterServiceIfNeeded();
    for (const observer of this.mObserverList) {
      if (sync) {
        observer.onReportEventSync?.(eventName, instanceId, propsData, extraData);
      } else {
        observer.onReportEvent(eventName, instanceId, propsData, extraData);
      }
    }
    TraceEvent.endSection(TraceCategory.Lynx, sectionName);
  }

  private onEventCallByNative(instanceId: InstanceId, eventName: string, props: Map<string, LynxReportEventPropValue>) {
    // currently, all the event from native should not be sync
    this.handleEvent(false, instanceId, eventName, props);
  }

  private updateGenericInfoCallByNative(instanceId: InstanceId, info: Record<string, LynxReportEventPropValue>) {
    const genericInfo = LynxEventReporter.getGenericInfoInternal(instanceId);
    Object.keys(info).forEach((key) => {
      genericInfo.set(key, info[key]);
    });
  }

  private clearCacheCallByNative(instanceId: InstanceId): void {
    if (instanceId < 0) {
      return;
    }
    LynxEventReporter.runOnReportThread(() => {
      this.mAllGenericInfos.delete(instanceId);
      this.mAllExtraParams.delete(instanceId);
    });
  }

  private setAll(from: ReadonlyMap<string, LynxReportEventPropValue> | Record<string, LynxReportEventPropValue>,
    to: Map<string, LynxReportEventPropValue>) {
    if (from instanceof Map) {
      (from as Map<string, LynxReportEventPropValue>).forEach((value, key) => {
        to.set(key, value);
      });
    } else {
      let record = from as Record<string, LynxReportEventPropValue>
      Object.keys(record).forEach((key) => {
        to.set(key, record[key]);
      });
    }
  }

  private setupEventReporterServiceIfNeeded() {
    if (this.eventReporterService) {
      return
    }
    this.eventReporterService =
      LynxServiceCenter.getService<ILynxEventReportObserver>(LynxServiceType.EventReporter);
    if (this.eventReporterService) {
      this.addObserver(this.eventReporterService);
    } else {
      LLog.e(LynxEventReporter.TAG, "eventReporter service not found or event name is null.");
    }
  }
}

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxExtraTiming } from './timing/LynxExtraTiming';
import { ITimingCollector } from './timing/ITimingCollector';
import lynx from 'liblynx.so';
import { IPerformanceObserver } from './IPerformanceObserver';
import { ILynxEventReporterService, LynxReportEventPropValue, LynxServiceCenter, LynxServiceType } from '../../Index';
import { PerformanceEntryConverter } from '../gen/performance/performanceobserver/PerformanceEntryConverter';
import { LynxEventReporter } from '../eventreport/LynxEventReporter';


export class PerformanceController implements ITimingCollector {
  public readonly native: lynx.PerformanceController;
  private observers?: IPerformanceObserver[];
  private reporterService?: ILynxEventReporterService;
  private isDestroyed: boolean = false;

  constructor() {
    this.native = new lynx.PerformanceController(this, this.onPerformanceEvent);
  }

  public setPerformanceObserver(observer: IPerformanceObserver[]): void {
    this.observers = observer;
  }

  public onPerformanceEvent(entry: Record<string, Object>): void {
    this.observers?.forEach(observer => observer.onPerformanceEvent(PerformanceEntryConverter.makePerformanceEntry(entry)));

    let service = this.getEventReporterService();
    if (service != undefined) {
      let instanceId: number = entry["instanceId"] as number;
      let genericInfo: Map<string, LynxReportEventPropValue> = LynxEventReporter.getGenericInfo(instanceId);
      // merge genericInfo and performance entry
      genericInfo.forEach((value, key) => {
        entry[key] = value
      });
      service.onPerformanceEvent(PerformanceEntryConverter.makePerformanceEntry(entry));
    }
  }
  
  public destroy(): void {
    this.isDestroyed = true;
    this.native.destroy();
  }

  private getEventReporterService(): ILynxEventReporterService | undefined {
    if (this.reporterService == undefined) {
      let service: ILynxEventReporterService | undefined =
        LynxServiceCenter.getService<ILynxEventReporterService>(LynxServiceType.EventReporter);
      if (service != undefined) {
        this.reporterService = service;
      }
    }
    return this.reporterService;
  }

  public setTiming(timestamp: number, timingKey: string, pipelineID?: string | undefined): void {
    if (this.isDestroyed) {
      return;
    }
    this.native.setTiming(timestamp, timingKey, pipelineID);
  }

  public markTiming(timingKey: string, pipelineID: string): void {
    if (this.isDestroyed) {
      return;
    }
    this.native.markTiming(timingKey, pipelineID);
  }

  public setExtraTiming(extraTiming: LynxExtraTiming) {
    if (!extraTiming) {
      return;
    }
    if (extraTiming.openTime > 0) {
      this.setTiming(extraTiming.openTime * 1000, LynxExtraTiming.OPEN_TIME);
    }
    if (extraTiming.containerInitStart > 0) {
      this.setTiming(extraTiming.containerInitStart * 1000, LynxExtraTiming.CONTAINER_INIT_START);
    }
    if (extraTiming.containerInitEnd > 0) {
      this.setTiming(extraTiming.containerInitEnd * 1000, LynxExtraTiming.CONTAINER_INIT_END);
    }
    if (extraTiming.prepareTemplateStart > 0) {
      this.setTiming(extraTiming.prepareTemplateStart * 1000, LynxExtraTiming.PREPARE_TEMPLATE_START);
    }
    if (extraTiming.prepareTemplateEnd > 0) {
      this.setTiming(extraTiming.prepareTemplateEnd * 1000, LynxExtraTiming.PREPARE_TEMPLATE_END);
    }
  }
}

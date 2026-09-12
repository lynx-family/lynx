// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { TimingConstants } from './TimingConstants';

export type EmbeddedTimingEntryCallback = (entry: Record<string, Object>) => void;

/** Collects pipeline timing when embedded mode does not use the native performance controller. */
export class EmbeddedTimingCollector {
  private createLynxStartUs: number = -1;
  private createLynxEndUs: number = -1;
  private loadBundleStartUs: number = -1;
  private loadBundleEndUs: number = -1;
  private updateDataStartUsList: number[] = [];
  private nextUpdateId: number = 0;
  private paintEndUs: number = -1;
  private hasEmittedLoadBundleEvent: boolean = false;
  private readonly entryCallback: EmbeddedTimingEntryCallback;

  constructor(entryCallback: EmbeddedTimingEntryCallback) {
    this.entryCallback = entryCallback;
  }

  public markTiming(key: string, timestampUs: number): void {
    switch (key) {
      case TimingConstants.CREATE_LYNX_START:
        this.createLynxStartUs = timestampUs;
        break;
      case TimingConstants.CREATE_LYNX_END:
        this.createLynxEndUs = timestampUs;
        break;
      case TimingConstants.LOAD_BUNDLE_START:
        this.loadBundleStartUs = timestampUs;
        this.emitLoadBundleIfReady();
        break;
      case TimingConstants.LOAD_BUNDLE_END:
        this.loadBundleEndUs = timestampUs;
        this.emitLoadBundleIfReady();
        break;
      case TimingConstants.UPDATE_DATA_START:
        this.updateDataStartUsList.push(timestampUs);
        break;
      case TimingConstants.PAINT_END:
        this.paintEndUs = timestampUs;
        this.emitLoadBundleIfReady();
        this.emitUpdateDataIfReady();
        break;
      default:
        break;
    }
  }

  private emitLoadBundleIfReady(): void {
    if (this.hasEmittedLoadBundleEvent || this.loadBundleStartUs < 0 ||
      this.loadBundleEndUs < this.loadBundleStartUs ||
      this.paintEndUs < this.loadBundleEndUs) {
      return;
    }
    this.hasEmittedLoadBundleEvent = true;
    const entry: Record<string, Object> = {};
    entry['entryType'] = 'pipeline';
    entry['name'] = TimingConstants.LOAD_BUNDLE;
    if (this.createLynxStartUs >= 0 && this.createLynxEndUs >= 0) {
      entry[TimingConstants.CREATE_LYNX_START] = this.createLynxStartUs / 1000;
      entry[TimingConstants.CREATE_LYNX_END] = this.createLynxEndUs / 1000;
    }
    entry[TimingConstants.LOAD_BUNDLE_START] = this.loadBundleStartUs / 1000;
    entry[TimingConstants.LOAD_BUNDLE_END] = this.loadBundleEndUs / 1000;
    entry[TimingConstants.PIPELINE_START] = this.loadBundleStartUs / 1000;
    entry[TimingConstants.PAINT_END] = this.paintEndUs / 1000;
    // Keep the pipeline end field for consumers that calculate pipeline duration.
    entry[TimingConstants.PIPELINE_END] = this.paintEndUs / 1000;
    this.entryCallback(entry);
  }

  private emitUpdateDataIfReady(): void {
    if (this.paintEndUs < 0 || this.updateDataStartUsList.length === 0) {
      return;
    }
    const updateDataStartUsList = this.updateDataStartUsList.filter(
      (updateDataStartUs: number) => updateDataStartUs <= this.paintEndUs);
    this.updateDataStartUsList = this.updateDataStartUsList.filter(
      (updateDataStartUs: number) => updateDataStartUs > this.paintEndUs);
    updateDataStartUsList.forEach((updateDataStartUs: number) => {
      const entry: Record<string, Object> = {};
      entry['entryType'] = 'pipeline';
      entry['name'] = TimingConstants.UPDATE_TRIGGERED_BY_NATIVE;
      entry['identifier'] = `embedded-update-${this.nextUpdateId++}`;
      entry[TimingConstants.PIPELINE_START] = updateDataStartUs / 1000;
      entry[TimingConstants.PAINT_END] = this.paintEndUs / 1000;
      entry[TimingConstants.PIPELINE_END] = this.paintEndUs / 1000;
      this.entryCallback(entry);
    });
  }
}

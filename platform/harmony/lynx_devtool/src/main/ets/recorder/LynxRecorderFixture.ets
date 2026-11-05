// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import fs from '@ohos.file.fs';
import { LLog } from '@lynx/lynx';
import { LynxDevToolEnvHarmony } from 'liblynxdevtool.so';
import { ActionData } from './DataType';
import { ReplaySharedData } from './ReplayDataModule';

interface FixtureAction {
  functionName: string;
  delayMs: number;
  params: Record<string, Object>;
}

interface FixtureEvaluation {
  actions: FixtureAction[];
  sharedData: ReplaySharedData;
}

// Assets outlive both the view and the C++ FixtureContext. The latter acquires
// its own reference through getFixtureDirectory() and releases it on Destroy.
export default class LynxRecorderFixture {
  static readonly MAX_BYTES: number = 128 * 1024 * 1024;
  readonly directory: string;
  readonly data: string;
  readonly sharedData: ReplaySharedData;
  private readonly root: string;
  private references: number = 1;

  private constructor(root: string, result: FixtureEvaluation) {
    this.root = root;
    this.directory = root + '/content';
    this.sharedData = result.sharedData;
    let actions: ActionData[] = [];
    for (const action of result.actions) {
      actions.push({ 'Function Name': action.functionName, RecordMillisecond: action.delayMs, Params: action.params });
    }
    let record: Record<string, Object> = {};
    let callbacks: Record<string, Object> = {};
    record['Action List'] = actions;
    record['Invoked Method Data'] = [];
    record['Callback'] = callbacks;
    record['SharedData'] = result.sharedData;
    this.data = JSON.stringify(record);
  }

  static isArchive(buffer: ArrayBuffer): boolean {
    const bytes = new Uint8Array(buffer);
    return bytes.length >= 2 && bytes[0] === 0x50 && bytes[1] === 0x4b;
  }

  static async open(buffer: ArrayBuffer, cache: string): Promise<LynxRecorderFixture> {
    if (buffer.byteLength > LynxRecorderFixture.MAX_BYTES) {
      throw new Error('Fixture archive exceeds size limit');
    }
    const root = fs.mkdtempSync(cache + '/lynx-fixture-XXXXXX');
    try {
      const archive = root + '/fixture.zip';
      fs.mkdirSync(root + '/content');
      let file = await fs.open(archive, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY);
      try {
        if (await fs.write(file.fd, buffer) !== buffer.byteLength) {
          throw new Error('Cannot write fixture archive');
        }
      } finally {
        await fs.close(file);
      }
      const result: FixtureEvaluation = JSON.parse(await LynxDevToolEnvHarmony.loadFixture(archive, root + '/content'));
      fs.unlinkSync(archive);
      return new LynxRecorderFixture(root, result);
    } catch (error) {
      LynxRecorderFixture.remove(root);
      throw error as Error;
    }
  }

  retain(): void {
    if (this.references === 0) {
      throw new Error('Fixture already released');
    }
    this.references++;
  }

  release(): void {
    if (this.references > 0 && --this.references === 0) {
      LynxRecorderFixture.remove(this.root);
    }
  }

  private static remove(root: string): void {
    try {
      fs.rmdirSync(root);
    } catch (error) {
      LLog.e('LynxRecorderFixture', 'Cannot remove fixture directory: ' + error);
    }
  }
}

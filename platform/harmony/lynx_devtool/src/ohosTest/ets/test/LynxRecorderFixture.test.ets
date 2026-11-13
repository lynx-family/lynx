// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { beforeAll, describe, expect, it } from '@ohos/hypium';
import { abilityDelegatorRegistry } from '@kit.TestKit';
import fs from '@ohos.file.fs';
import { util } from '@kit.ArkTS';
import { LynxContext, LynxEnv } from '@lynx/lynx';
import LynxRecorderFixture from '../../../main/ets/recorder/LynxRecorderFixture';
import LynxRecorderReplayDataModule, {
  LynxRecorderFixtureReplayDataModule, ReplaySharedData
} from '../../../main/ets/recorder/ReplayDataModule';
import { ActionData } from '../../../main/ets/recorder/DataType';

// A ZIP containing fixture.js and assets/page.bin. The first action is delayed
// by 75 ms to catch accidental normalization to JSON's first timestamp.
// cspell:disable-next-line
const ARCHIVE = 'UEsDBBQAAAAIAAAAIVxySJi9qwAAAP4AAAAKAAAAZml4dHVyZS5qc1WNuwrDMAxF936FNzsQ8hhCwaWd+gmFzqqjNKGObWwZAsH/XqdkSAehy5E4FxdnPbEeB4ia2BCNoskaoWgp2HpiLIcKBkIvzl3JRMGutx/TFvoHzk4DoeAjkQuyrnGBjLBSdq4dvJGXjG+7ek0m59V+JPmIqSguf+62aY7yfgoOSI2CH2s2QbDRK5Qcnl3IozOj/XwHArmmgzuM4LHfuOCBdoOy0ZBsU/5KX1BLAwQUAAAACAAAACFcgx9glwoAAAAIAAAADwAAAGFzc2V0cy9wYWdlLmJpbitJzS3ISSxJBQBQSwECFAMUAAAACAAAACFcckiYvasAAAD+AAAACgAAAAAAAAAAAAAAgAEAAAAAZml4dHVyZS5qc1BLAQIUAxQAAAAIAAAAIVyDH2CXCgAAAAgAAAAPAAAAAAAAAAAAAACAAdMAAABhc3NldHMvcGFnZS5iaW5QSwUGAAAAAAIAAgB1AAAACgEAAAAA';

export default function LynxRecorderFixtureTest() {
  describe('LynxRecorderFixtureTest', () => {
    beforeAll(() => {
      LynxEnv.initialize(abilityDelegatorRegistry.getAbilityDelegator().getAppContext());
    });

    it('keepsAssetsUntilTheViewAndContextReleaseThem', 0, async () => {
      const cache = abilityDelegatorRegistry.getAbilityDelegator().getAppContext().cacheDir;
      const bytes = new util.Base64Helper().decodeSync(ARCHIVE);
      const fixture = await LynxRecorderFixture.open(bytes.buffer as ArrayBuffer, cache);
      const module = new LynxRecorderFixtureReplayDataModule(new LynxContext(), fixture);
      try {
        expect(module.getFixtureDirectory()).assertEqual(fixture.directory);
        expect(module.getFixtureDirectory()).assertEqual(fixture.directory);
        const data: Record<string, Object> = JSON.parse(fixture.data);
        expect(Object.keys(data).includes('Config')).assertFalse();
        expect(Object.keys(data).includes('Component List')).assertFalse();
        const actions = data['Action List'] as ActionData[];
        expect(actions[0].RecordMillisecond).assertEqual(75);
        const params = actions[0].Params as Record<string, Object>;
        // cspell:disable-next-line
        expect(params['source']).assertEqual('dGVtcGxhdGU=');
        const inlineParams = actions[1].Params as Record<string, Object>;
        expect(actions[1].RecordMillisecond).assertEqual(100);
        expect(inlineParams['source']).assertEqual('aW5saW5l');
        const shared = module.getSharedData('state').value as Record<string, Object>;
        expect(shared['count']).assertEqual(1);
        fixture.release();
        expect(fs.accessSync(fixture.directory + '/assets/page.bin')).assertTrue();
        module.releaseFixture();
        expect(fs.accessSync(fixture.directory)).assertFalse();
        module.releaseFixture();
      } finally {
        module.releaseFixture();
        fixture.release();
      }
    });

    it('keepsJsonReplayWithoutAFixture', 0, () => {
      const module = new LynxRecorderReplayDataModule(new LynxContext());
      // The native module bridge discovers own prototype methods. JSON must not
      // expose the getter: even an empty result requires a synchronous UI hop.
      const methods = Object.getOwnPropertyNames((LynxRecorderReplayDataModule as Function).prototype);
      expect(methods.includes('getFixtureDirectory')).assertFalse();
      expect(methods.includes('releaseFixture')).assertFalse();
      expect(methods.includes('getData')).assertTrue();
      expect(methods.includes('getSharedData')).assertTrue();
      expect(LynxRecorderReplayDataModule.syncMethods.includes('getFixtureDirectory')).assertFalse();
      LynxRecorderReplayDataModule.jsbInvokers = [];
      const shared: ReplaySharedData = {};
      shared['state'] = 'json';
      LynxRecorderReplayDataModule.sharedData = shared;
      expect(module.getSharedData('state').value).assertEqual('json');
      module.getData((text: string) => {
        const data: Record<string, string> = JSON.parse(text);
        expect(data['RecordData']).assertEqual('{}');
      });
      LynxRecorderReplayDataModule.sharedData = {};
    });

    it('exposesFixtureMethodsToTheNativeBridge', 0, () => {
      const methods = Object.getOwnPropertyNames((LynxRecorderFixtureReplayDataModule as Function).prototype);
      expect(methods.includes('getFixtureDirectory')).assertTrue();
      expect(methods.includes('releaseFixture')).assertTrue();
      expect(methods.includes('getSharedData')).assertTrue();
      expect(LynxRecorderFixtureReplayDataModule.syncMethods.includes('getFixtureDirectory')).assertTrue();
      expect(LynxRecorderFixtureReplayDataModule.syncMethods.includes('releaseFixture')).assertFalse();
    });

    it('cleansUpAFailedArchive', 0, async () => {
      const cache = abilityDelegatorRegistry.getAbilityDelegator().getAppContext().cacheDir;
      const root = fs.mkdtempSync(cache + '/fixture-failure-XXXXXX');
      let failed = false;
      try {
        try {
          await LynxRecorderFixture.open(new Uint8Array([0x50, 0x4b, 0, 0]).buffer as ArrayBuffer, root);
        } catch (error) {
          failed = true;
        }
        expect(failed).assertTrue();
        expect(fs.listFileSync(root).length).assertEqual(0);
      } finally {
        fs.rmdirSync(root);
      }
    });
  });
}

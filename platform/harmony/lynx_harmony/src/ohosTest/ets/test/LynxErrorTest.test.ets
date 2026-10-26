// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expect, it } from '@ohos/hypium';
import { LynxError } from '../../../main/ets/tasm/LynxError';

export default function LynxErrorTest() {
  describe('LynxErrorTest', () => {
    it('preservesRootCauseInGetterAndJson', 0, () => {
      const error = LynxError.createLynxError(30301, 'request failed', { src: 'script.js' });
      error.setRootCause('network unavailable');

      expect(error.getRootCause()).assertEqual('network unavailable');
      const json: Record<string, Object> = JSON.parse(error.getMsg() || '{}');
      expect(json['root_cause']).assertEqual('network unavailable');
      expect(json['src']).assertEqual('script.js');

      error.setRootCause('updated cause');
      const updated: Record<string, Object> = JSON.parse(error.getMsg() || '{}');
      expect(updated['root_cause']).assertEqual('updated cause');
    });

    it('omitsAnEmptyRootCause', 0, () => {
      const error = LynxError.createLynxError(30301, 'request failed');
      expect(error.getRootCause()).assertEqual('');
      error.setRootCause('network unavailable');
      error.getMsg();
      error.setRootCause('');
      const json: Record<string, Object> = JSON.parse(error.getMsg() || '{}');
      expect(Object.keys(json).includes('root_cause')).assertEqual(false);
    });

    it('preservesLegacyCustomRootCause', 0, () => {
      const error = LynxError.createLynxError(30301, 'request failed', { root_cause: 'legacy cause' });
      const json: Record<string, Object> = JSON.parse(error.getMsg() || '{}');
      expect(json['root_cause']).assertEqual('legacy cause');
      error.setRootCause('native cause');
      const updated: Record<string, Object> = JSON.parse(error.getMsg() || '{}');
      expect(updated['root_cause']).assertEqual('native cause');
    });
  });
}

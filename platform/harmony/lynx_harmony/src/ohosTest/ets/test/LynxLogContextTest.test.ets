// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expect, it } from '@ohos/hypium';
import { LynxLogContext } from '../../../main/ets/tasm/LynxLogContext';

export default function LynxLogContextTest() {
  describe('LynxLogContextTest', () => {
    it('formatsCanonicalTuple', 0, () => {
      const context = new LynxLogContext(-1, 0, 2147483647);

      expect(context.viewId).assertEqual(-1);
      expect(context.engineId).assertEqual(0);
      expect(context.runtimeId).assertEqual(2147483647);
      expect(context.toString()).assertEqual('[-1,0,2147483647]');
    });

    it('keepsCopiedSnapshotsImmutable', 0, () => {
      let current = new LynxLogContext(7, 8, 9);
      const published = current;
      current = new LynxLogContext(7, 10, 11);

      expect(published.toString()).assertEqual('[7,8,9]');
      expect(current.toString()).assertEqual('[7,10,11]');
    });
  });
}

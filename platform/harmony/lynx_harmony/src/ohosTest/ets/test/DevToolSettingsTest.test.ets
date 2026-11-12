// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { afterEach, beforeEach, describe, expect, it } from '@ohos/hypium';
import { DevToolSettings } from '../../../main/ets/devtool_wrapper/DevToolSettings';

export default function DevToolSettingsTest() {
  describe('DevToolSettingsTest', () => {
    beforeEach(() => {
      DevToolSettings.setEnabledCDPDomains([]);
    });

    afterEach(() => {
      DevToolSettings.setEnabledCDPDomains([]);
    });

    it('enablesAndDisablesIndividualDomains', 0, () => {
      expect(DevToolSettings.getEnabledCDPDomains().size).assertEqual(0);
      DevToolSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS, true);
      DevToolSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM, true);
      DevToolSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS, false);
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS)).assertFalse();
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM)).assertTrue();
    });

    it('takesSnapshotsOfInputAndOutputSets', 0, () => {
      const domains = new Set<string>([DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM]);
      DevToolSettings.setEnabledCDPDomains(domains);
      // Mutating input or output sets must not change settings.
      domains.clear();
      DevToolSettings.getEnabledCDPDomains().clear();
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM)).assertTrue();
    });

    it('replacesDomainsAndRemovesDuplicates', 0, () => {
      DevToolSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM, true);
      const domains: string[] = [DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS,
        DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS];
      // Batch updates replace the previous set and ignore duplicates.
      DevToolSettings.setEnabledCDPDomains(domains);
      domains.length = 0;
      expect(DevToolSettings.getEnabledCDPDomains().size).assertEqual(1);
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM)).assertFalse();
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS)).assertTrue();
      DevToolSettings.setEnabledCDPDomains([]);
      expect(DevToolSettings.getEnabledCDPDomains().size).assertEqual(0);
    });

    it('rejectsInvalidKeysWithoutChangingState', 0, () => {
      DevToolSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM, true);
      // One invalid key rejects the entire batch, including CSS.
      DevToolSettings.setEnabledCDPDomains([DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS,
        DevToolSettings.SP_KEY_ENABLE_DEVTOOL]);
      // The single-domain setter must also reject non-CDP keys.
      DevToolSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, true);
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_DEVTOOL)).assertFalse();
      expect(DevToolSettings.getEnabledCDPDomains().size).assertEqual(1);
      expect(DevToolSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM)).assertTrue();
    });
  });
}

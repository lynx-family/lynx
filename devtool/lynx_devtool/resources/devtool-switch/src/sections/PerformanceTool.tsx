import { useAtom, useAtomValue } from 'jotai';

import { performanceTool, platform } from '../atoms';
import { Switch } from '../components/Switch';
import { i18n } from '../i18n';

export function PerformanceTool() {
  const [enable, setEnable] = useAtom(performanceTool);

  const currentPlatform = useAtomValue(platform);

  if (currentPlatform !== 'Android') {
    return null;
  }

  return (
    <Switch
      title={i18n.t('Performance Tool')}
      description={i18n.t('Performance Tool desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

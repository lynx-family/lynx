import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { timingOverlay } from '../atoms';

import { Switch } from '../components/Switch';

export function TimingOverlay() {
  const [enable, setEnable] = useAtom(timingOverlay);

  return (
    <Switch
      title={i18n.t('Timing Overlay')}
      description={i18n.t('Timing Overlay')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

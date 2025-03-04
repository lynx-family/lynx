import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { longPress } from '../atoms';

import { Switch } from '../components/Switch';

export function LongPress() {
  const [enable, setEnable] = useAtom(longPress);

  return (
    <Switch
      title={i18n.t('Longpress')}
      description={i18n.t('Longpress desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

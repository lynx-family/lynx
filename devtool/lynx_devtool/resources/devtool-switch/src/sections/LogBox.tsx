import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { logBox } from '../atoms';

import { Switch } from '../components/Switch';

export function LogBox() {
  const [enable, setEnable] = useAtom(logBox);

  return (
    <Switch
      title={i18n.t('LogBox')}
      description={i18n.t('LogBox desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

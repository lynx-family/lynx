import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { fspScreenshot } from '../atoms';

import { Switch } from '../components/Switch';

export function FSPScreenshot() {
  const [enable, setEnable] = useAtom(fspScreenshot);

  return (
    <Switch
      title={i18n.t('Enable FSP Screenshot')}
      description={i18n.t('Enable FSP Screenshot desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { lynxDebug } from '../atoms';

import { Switch } from '../components/Switch';

export function Header() {
  const [enable, setEnable] = useAtom(lynxDebug);

  return (
    <Switch
      title="Lynx Debug"
      description={i18n.t('Lynx Debug desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

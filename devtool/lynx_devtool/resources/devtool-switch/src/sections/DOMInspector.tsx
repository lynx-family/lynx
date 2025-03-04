import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { domInspect } from '../atoms';

import { Switch } from '../components/Switch';

export function DOMInspector() {
  const [enable, setEnable] = useAtom(domInspect);

  return (
    <Switch
      title={i18n.t('DOM inspector')}
      description={i18n.t('DOM inspector desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

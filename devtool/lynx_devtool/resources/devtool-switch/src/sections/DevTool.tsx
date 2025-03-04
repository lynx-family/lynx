import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { devtool } from '../atoms';

import { Switch } from '../components/Switch';

export function DevTool() {
  const [enable, setEnable] = useAtom(devtool);

  return (
    <Switch
      title="Lynx DevTool"
      description={i18n.t('Lynx DevTool desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

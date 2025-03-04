import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { highlightTouch } from '../atoms';

import { Switch } from '../components/Switch';

export function HighlightTouch() {
  const [enable, setEnable] = useAtom(highlightTouch);

  return (
    <Switch
      title={i18n.t('Highlight Touch')}
      description={i18n.t('Highlight Touch desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

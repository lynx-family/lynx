import { useAtomValue } from 'jotai';

import { i18n } from '../i18n';
import { lynxDebug } from '../atoms';

import { Status } from '../components/Status';

export function Header() {
  const enable = useAtomValue(lynxDebug);

  return (
    <Status
      title="DevTool Status"
      description={i18n.t('DevTool Status desc')}
      status={enable ? 'Enabled' : 'Disabled'}
    />
  );
}

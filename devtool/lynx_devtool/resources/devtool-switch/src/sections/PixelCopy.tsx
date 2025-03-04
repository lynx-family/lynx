import { useAtom } from 'jotai';

import { i18n } from '../i18n';
import { pixelCopy, platform } from '../atoms';

import { Switch } from '../components/Switch';
import { useAtomValue } from 'jotai';

export function PixelCopy() {
  const [enable, setEnable] = useAtom(pixelCopy);

  const currentPlatform = useAtomValue(platform);

  if (currentPlatform !== 'Android') {
    return null;
  }

  return (
    <Switch
      title={i18n.t('PixelCopy')}
      description={i18n.t('PixelCopy desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

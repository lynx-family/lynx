import { useAtomValue } from 'jotai';

import { devtool, lynxDebug, platform, quickjsDebug, v8 } from '../atoms';
import { i18n } from '../i18n';

import { Status } from '../components/Status';

export function CurrentJSEngine() {
  const enableDevTool = useAtomValue(devtool);
  const enableV8 = useAtomValue(v8);
  const enableQuickjsDebug = useAtomValue(quickjsDebug);
  const currentPlatform = useAtomValue(platform);
  const enableLynxDebug = useAtomValue(lynxDebug);

  const getCurrentJSEngine = () => {
    if (!enableLynxDebug || !enableDevTool) {
      return 'Unknown';
    }
    if (enableV8 === 1) {
      return 'V8';
    } else if (enableV8 == 0 && enableQuickjsDebug) {
      return 'PrimJS';
    } else if (enableV8 == 2 && currentPlatform == 'Android') {
      return 'enable_v8: V8\nother: PrimJS';
    }
    return 'Unknown';
  };
  return (
    <Status
      title={i18n.t('Current BTS Engine')}
      status={getCurrentJSEngine()}
      labelStyle={{ width: '50%', marginLeft: '0', paddingLeft: '30rpx' }}
      statusStyle={{ width: '50%', marginRight: '0', paddingRight: '30rpx' }}
    />
  );
}

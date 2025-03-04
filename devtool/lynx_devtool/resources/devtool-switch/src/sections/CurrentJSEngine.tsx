import { useAtomValue } from 'jotai';

import { devtool, lynxDebug, platform, quickjsDebug, v8 } from '../atoms';
import { i18n } from '../i18n';

export function CurrentJSEngine() {
  const enableDevTool = useAtomValue(devtool);
  const enableV8 = useAtomValue(v8);
  const enableQuickjsDebug = useAtomValue(quickjsDebug);
  const currentPlatform = useAtomValue(platform);
  const enableLynxDebug = useAtomValue(lynxDebug);

  const getCurrentJSEngine = () => {
    if (enableLynxDebug && enableDevTool && enableV8 === 1) {
      return 'V8';
    } else if (
      enableLynxDebug &&
      enableDevTool &&
      enableV8 == 0 &&
      enableQuickjsDebug
    ) {
      return 'PrimJS';
    } else {
      if (currentPlatform === 'Android') {
        return '\nenable_v8: V8, \nother: PrimJS';
      } else {
        return 'JSC';
      }
    }
  };
  return (
    <view className="item_wrap">
      <view className="label_wrap">
        <text className="label_text">
          {i18n.t('Current JS Engine')}: {getCurrentJSEngine()}
        </text>
      </view>
    </view>
  );
}

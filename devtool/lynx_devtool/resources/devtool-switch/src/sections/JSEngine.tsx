import { useAtom, useAtomValue } from 'jotai';

import { platform, quickjsCache, quickjsDebug, v8 } from '../atoms';
import { Switch } from '../components/Switch';
import { i18n } from '../i18n';
import { Radio } from '../components/Radio';

export function JSEngine() {
  const currentPlatform = useAtomValue(platform);
  if (currentPlatform == 'Android') {
    const v8Enable = useAtomValue(v8);
    return (
      <>
        <V8 />
        {v8Enable !== 1 && <QuickJS />}
      </>
    );
  } else {
    return (
      <>
        <QuickJS />
      </>
    );
  }
}

function V8() {
  const [v8Enable, setV8Enable] = useAtom(v8);
  const currentPlatform = useAtomValue(platform);

  return (
    <Radio
      on={v8Enable}
      onChange={(index) => setV8Enable(index as 0 | 1 | 2)}
      title={i18n.t('V8')}
      description={i18n.t('V8 desc')}
      values={['Off', 'On', 'AlignWithProd']}
    />
  );
}

function QuickJS() {
  return (
    <>
      <QuickJSDebug />
      <QuickJSCache />
    </>
  );
}

function QuickJSDebug() {
  const currentPlatform = useAtomValue(platform);
  const [enable, setEnable] = useAtom(quickjsDebug);

  return (
    <Switch
      title={i18n.t('QuickJS debug')}
      description={
        currentPlatform === 'Android'
          ? i18n.t('QuickJS debug desc Android')
          : i18n.t('QuickJS debug desc iOS')
      }
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

function QuickJSCache() {
  const currentPlatform = useAtomValue(platform);
  const enableQuickjsDebug = useAtomValue(quickjsDebug);
  const [enable, setEnable] = useAtom(quickjsCache);

  if (currentPlatform !== 'Android' || enableQuickjsDebug) {
    return null;
  }

  return (
    <Switch
      title={i18n.t('QuickJS cache')}
      description={i18n.t('QuickJS cache desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}

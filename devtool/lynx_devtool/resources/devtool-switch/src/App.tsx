import { useAtomValue } from 'jotai';

import { devtool, lynxDebug } from './atoms';
import { DevTool } from './sections/DevTool';
import { DOMInspector } from './sections/DOMInspector';
import { JSEngine } from './sections/JSEngine';
import { LongPress } from './sections/LongPress';
import { Header } from './sections/Header';
import { HighlightTouch } from './sections/HighlightTouch';
import { LogBox } from './sections/LogBox';
import { TestBench } from './sections/TestBench';
import { PixelCopy } from './sections/PixelCopy';
import { CurrentJSEngine } from './sections/CurrentJSEngine';
import { FSPScreenshot } from './sections/FSPScreenshot';

export default function App() {
  const safeArea = getSafeArea();
  const horizontalSafeArea = safeArea.left + safeArea.right;

  return (
    <view className="container" style="height:100%;width:100%">
      <scroll-view scroll-y style="height:100%;width:100%">
        <view
          style={{
            marginLeft: `${safeArea.left}px`,
            marginRight: `${safeArea.right}px`,
            width: `calc(100% - ${horizontalSafeArea}px)`,
            paddingBottom: `${safeArea.bottom}px`,
          }}
        >
          <Header />
          <LynxDebug experimental={true} />
          <CurrentJSEngine />
        </view>
      </scroll-view>
    </view>
  );
}

function getSafeArea() {
  const globalProps = lynx.__globalProps || {};
  const screenWidth = Number(globalProps.screenWidth || 0);
  const screenHeight = Number(globalProps.screenHeight || 0);
  const isPortrait =
    screenWidth > 0 && screenHeight > 0 ? screenHeight >= screenWidth : true;
  const isIOS = SystemInfo.platform === 'iOS';
  const isNotchScreen =
    globalProps.isNotchScreen ||
    (globalProps.safeAreaTop || 0) > 20 ||
    (globalProps.safeAreaBottom || 0) > 0 ||
    (globalProps.safeAreaLeft || 0) > 0 ||
    (globalProps.safeAreaRight || 0) > 0;
  const landscapeSideFallback = isIOS && isNotchScreen ? 54 : 0;

  return {
    top: Math.max(
      globalProps.safeAreaTop || 0,
      isPortrait && isNotchScreen ? 54 : 0
    ),
    bottom: Math.max(
      globalProps.safeAreaBottom || 0,
      isPortrait && isNotchScreen ? 34 : 0
    ),
    left: Math.max(
      globalProps.safeAreaLeft || 0,
      isPortrait ? 0 : landscapeSideFallback
    ),
    right: Math.max(
      globalProps.safeAreaRight || 0,
      isPortrait ? 0 : landscapeSideFallback
    ),
  };
}

interface Props {
  experimental: boolean;
}

export function LynxDebug(props: Props) {
  const enableLynxDebug = useAtomValue(lynxDebug);

  if (!enableLynxDebug) {
    return null;
  }
  return (
    <>
      <DevTool />
      <LynxDevTool experimental={props.experimental} />
      <LogBox />
      <TestBench />
    </>
  );
}

function LynxDevTool(props: Props) {
  const enableDevTool = useAtomValue(devtool);

  if (!enableDevTool) {
    return null;
  }

  if (props.experimental) {
    return (
      <>
        <JSEngine />
        <LongPress />
        <DOMInspector />
        <PixelCopy />
      </>
    );
  }
  return (
    <>
      <JSEngine />
      <LongPress />
      <DOMInspector />
      <PixelCopy />
      <HighlightTouch />
      <FSPScreenshot />
    </>
  );
}

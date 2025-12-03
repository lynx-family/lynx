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
  return (
    <view className="container" style="height:100%;width:100%">
      <scroll-view scroll-y style="height:100%;width:100%">
        <Header />
        <LynxDebug experimental={true} />
        <CurrentJSEngine />
      </scroll-view>
    </view>
  );
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

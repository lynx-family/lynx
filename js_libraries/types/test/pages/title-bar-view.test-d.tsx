import { expectType } from 'tsd';
import { TitleBarViewProps, IntrinsicElements } from '../../types';

// Props Types Check
let a: unknown;
{
  <title-bar-view moveable={true} />;
  expectType<boolean | undefined>(a as IntrinsicElements['title-bar-view']['moveable']);
}

// Events types check
function noop() {}
{
  <title-bar-view bindtap={noop} />;
}

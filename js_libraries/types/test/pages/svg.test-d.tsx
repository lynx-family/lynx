import { expectType } from 'tsd';
import { SVGProps, IntrinsicElements, BaseEvent } from '../../types';

// Props Types Check
let a: unknown;
{
  <svg src="https://example.com/svg.svg" />;
  expectType<string | undefined>(a as IntrinsicElements['svg']['src']);
  <svg
    content={`
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
      <circle cx="50" cy="50" r="40" stroke="green" stroke-width="4" fill="yellow" />
    </svg>
  `}
  />;
  expectType<string | undefined>(a as IntrinsicElements['svg']['content']);
  <svg current-color="#ff0000" />;
  expectType<string | undefined>(a as IntrinsicElements['svg']['current-color']);
}

// Events types check
function noop() {}
{
  <svg bindtap={noop} />;
  <svg bindfocus={(e: BaseEvent) => {}} />;
}

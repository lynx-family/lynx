import type * as CSS from 'csstype';
import { assertType, describe, expectTypeOf, it } from 'vitest';
import {
  CompatibleCSSProperties,
  CSSProperties,
  CSSPropertiesWithLonghands,
  CSSPropertiesWithShorthands,
  InlineStyleProperties,
  LynxCSSProperties,
  StrictCSSProperties,
  StrictStyleSheet,
} from '../../types';
import type { PreviousCSSProperties } from './fixtures/css-properties-head';

declare const css: CSSProperties;
declare const cssWithLonghands: CSSPropertiesWithLonghands;
declare const cssWithShorthands: CSSPropertiesWithShorthands;
declare const standardCSSProperties: CSS.Properties<string | number>;
declare const lynxCSSProperties: LynxCSSProperties;
declare const previousCSSProperties: PreviousCSSProperties;

describe('CSSProperty Type Test', () => {
  it('example', () => {
    assertType<CSSProperties>({
      // layout
      flexFlow: '1',
      marginInlineStart: '1px',
      marginInlineEnd: '1px',
      paddingInlineStart: '1px',
      paddingInlineEnd: '1px',
      // typography
      outline: '1px solid red',
      textDecoration: '1px',
      XAutoFontSizeLineRanges: 'line-range(1 to infinity, 12px, 18px)',
      XCaretGradient: 'linear-gradient(180deg, #ffffff 0%, #7dd3fc 100%)',
      XCaretWidth: '2px',
      XCaretHeight: 16,
      XCaretRadius: 4,
      // visual
      border: '1px',
      borderRight: '1px',
      borderLeft: '1px',
      borderTop: '1px',
      borderBottom: '1px',
      // animation
      transition: '1px',
      transitionProperty: 'margin',
      transitionDuration: '1s',
      transitionDelay: '1s',
      transitionTimingFunction: 'ease-in',
      // other
      top: '1px',
      visibility: 'hidden',
      content: '1px',
      overflowX: 'hidden',
      overflowY: 'hidden',
      wordBreak: 'normal',
      verticalAlign: 'baseline',
      direction: 'normal',
      pointerEvents: 'auto',
    });

    assertType<CSSProperties>({
      pointerEvents: 'none',
    });

    assertType<CSSProperties>({
      // @ts-expect-error: pointerEvents does not accept arbitrary values
      pointerEvents: 'xxx',
    });

    assertType<CSSPropertiesWithLonghands>({
      // layout
      marginInlineStart: '1px',
      marginInlineEnd: '1px',
      paddingInlineStart: '1px',
      paddingInlineEnd: '1px',
      // typography
      outlineColor: 'red',
      XAutoFontSizeLineRanges: 'line-range(1 to infinity, 12px, 18px)',
      XCaretGradient: 'linear-gradient(180deg, #ffffff 0%, #7dd3fc 100%)',
      XCaretWidth: 2,
      XCaretHeight: '16px',
      XCaretRadius: '4px',
      // visual
      borderBottomLeftRadius: '1px',
      // animation
      transitionProperty: 'margin',
      transitionDuration: '1s',
      transitionDelay: '1s',
      transitionTimingFunction: 'ease-in',
      // other
      top: '1px',
      visibility: 'hidden',
      content: '1px',
      overflowX: 'hidden',
      overflowY: 'hidden',
      wordBreak: 'normal',
      verticalAlign: 'baseline',
      direction: 'normal',
      pointerEvents: 'auto',
    });

    assertType<CSSPropertiesWithShorthands>({
      // layout
      flexFlow: '1',
      // typography
      outline: '1px',
      textDecoration: '1px',
      // visual
      border: '1px',
      borderRight: '1px',
      borderLeft: '1px',
      borderTop: '1px',
      borderBottom: '1px',
      // animation
      transition: '1px',
      // other
      overflow: 'hidden',
    });
  });

  it('cssWithLonghands and cssWithShorthands should be assignable to CSSProperties', () => {
    assertType<CSSProperties>(cssWithLonghands);
    assertType<CSSProperties>(cssWithShorthands);

    assertType<CSSPropertiesWithLonghands>(css);

    assertType<CSSPropertiesWithShorthands>(css);
  });

  it('preserves the complete csstype compatibility surface', () => {
    assertType<CSSProperties>(standardCSSProperties);
    assertType<CSSProperties>(previousCSSProperties);

    assertType<CSSProperties>({
      flex: 1,
      aspectRatio: 4 / 3,
      top: 0,
      right: 0,
      bottom: 0,
      left: 0,
      zIndex: '0',
      flexShrink: '0',
      backgroundOrigin: 'border-box, padding-box',
      backgroundRepeat: 'no-repeat, repeat',
      paddingLeft: 10,
      marginTop: 20,
      layoutAnimationCreateProperty: 'width',
      listMainAxisGap: '12px',
    });

    assertType<CSSProperties>({
      // @ts-expect-error: unknown property names are not valid inline styles
      definitelyNotAStyle: 'value',
    });
  });

  it('offers generated Lynx style checking as an opt-in', () => {
    assertType<LynxCSSProperties>({
      display: 'linear',
      position: 'absolute',
      gridColumnSpan: 2,
      pointerEvents: 'none',
      transformOrigin: 'center',
      XCaretWidth: 2,
      XCaretHeight: 16,
      XCaretRadius: 4,
      listMainAxisGap: '12px',
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: static is outside the generated Lynx position values
      position: 'static',
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: backdrop-filter is not supported by Lynx metadata
      backdropFilter: 'blur(4px)',
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: arbitrary strings are outside generated keyword values
      transformOrigin: 'not-a-transform-origin',
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: unitless numbers are outside strict padding metadata
      paddingLeft: 10,
      // @ts-expect-error: unitless numbers are outside strict margin metadata
      marginTop: 20,
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: arbitrary properties are outside generated animation values
      layoutAnimationCreateProperty: 'width',
    });

    assertType<CSSProperties>(lynxCSSProperties);
  });

  it('supports scoped strict style adoption', () => {
    expectTypeOf<CompatibleCSSProperties>().toEqualTypeOf<CSSProperties>();
    expectTypeOf<StrictCSSProperties>().toEqualTypeOf<LynxCSSProperties>();
    expectTypeOf<InlineStyleProperties>().toEqualTypeOf<CSSProperties>();

    const styles = {
      root: {
        display: 'linear',
        position: 'absolute',
      },
    } satisfies StrictStyleSheet;

    expectTypeOf(styles.root.display).toEqualTypeOf<'linear'>();
    expectTypeOf(styles.root.position).toEqualTypeOf<'absolute'>();

    assertType<StrictStyleSheet>({
      invalid: {
        // @ts-expect-error: compatible-only values remain outside strict metadata
        position: 'static',
      },
    });
  });
});

import { assertType, describe, expectTypeOf, it } from 'vitest';
import { CSSProperties, CSSPropertiesWithLonghands, CSSPropertiesWithShorthands } from '../../types';

declare const css: CSSProperties;
declare const cssWithLonghands: CSSPropertiesWithLonghands;
declare const cssWithShorthands: CSSPropertiesWithShorthands;

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
    });
    assertType<CSSPropertiesWithLonghands>({
      // layout
      marginInlineStart: '1px',
      marginInlineEnd: '1px',
      paddingInlineStart: '1px',
      paddingInlineEnd: '1px',
      // typography
      outlineColor: 'red',
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
});

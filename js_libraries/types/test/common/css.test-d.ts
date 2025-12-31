import { assertType, describe, expectTypeOf, it } from 'vitest';
import {
  CSSProperties,
  CSSPropertiesWithLonghands,
  CSSPropertiesWithShorthands,
  LynxCSSProperties,
  LynxCSSPropertiesWithLonghands,
  LynxCSSPropertiesWithShorthands,
} from '../../types';

declare const css: CSSProperties;
declare const cssWithLonghands: CSSPropertiesWithLonghands;
declare const cssWithShorthands: CSSPropertiesWithShorthands;
declare const lynxCss: LynxCSSProperties;
declare const lynxCssWithLonghands: LynxCSSPropertiesWithLonghands;
declare const lynxCssWithShorthands: LynxCSSPropertiesWithShorthands;

describe('CSSProperties (Loose Mode) Type Test', () => {
  it('should accept Lynx-specific property values', () => {
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
      pointerEvents: 'auto',
    });

    assertType<CSSProperties>({
      pointerEvents: 'none',
    });

    assertType<CSSProperties>({
      // @ts-expect-error: pointerEvents only accept 'auto' or 'none'
      pointerEvents: 'xxx',
    });
  });

  it('should accept standard CSS properties from csstype (web compatibility)', () => {
    // These properties are NOT in Lynx's css_defines but should still work
    // because CSSProperties extends CSS.Properties from csstype
    assertType<CSSProperties>({
      color: 'red',
      backgroundColor: '#fff',
      fontSize: '16px',
      fontFamily: 'Arial',
      margin: '10px',
      padding: '20px',
      // Standard CSS values that may not be fully supported by Lynx runtime
      // but should still be type-valid for migration purposes
      boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
      textTransform: 'uppercase',
    });
  });

  it('should work with longhand properties', () => {
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
      pointerEvents: 'auto',
    });
  });

  it('should work with shorthand properties', () => {
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

  it('longhands and shorthands should be assignable to CSSProperties', () => {
    assertType<CSSProperties>(cssWithLonghands);
    assertType<CSSProperties>(cssWithShorthands);

    assertType<CSSPropertiesWithLonghands>(css);
    assertType<CSSPropertiesWithShorthands>(css);
  });
});

describe('LynxCSSProperties (Strict Mode) Type Test', () => {
  it('should accept only Lynx-supported properties and values', () => {
    assertType<LynxCSSProperties>({
      display: 'flex',
      flexDirection: 'row',
      justifyContent: 'center',
      alignItems: 'center',
      padding: '10px',
      margin: '20px',
    });
  });

  it('should accept Lynx-specific display values', () => {
    // Lynx has specific display values like 'linear' and 'relative'
    assertType<LynxCSSProperties>({
      display: 'linear',
    });

    assertType<LynxCSSProperties>({
      display: 'relative',
    });

    assertType<LynxCSSProperties>({
      display: 'auto',
    });
  });

  it('should accept Lynx-specific layout properties', () => {
    assertType<LynxCSSProperties>({
      linearOrientation: 'horizontal',
      linearWeight: 1,
      linearGravity: 'center',
      linearLayoutGravity: 'stretch',
      linearCrossGravity: 'center',
    });
  });

  it('should reject unsupported display values in strict mode', () => {
    assertType<LynxCSSProperties>({
      // @ts-expect-error: 'inline' is not supported in Lynx
      display: 'inline',
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: 'inline-block' is not supported in Lynx
      display: 'inline-block',
    });

    assertType<LynxCSSProperties>({
      // @ts-expect-error: 'table' is not supported in Lynx
      display: 'table',
    });
  });

  it('should support grid properties', () => {
    assertType<LynxCSSProperties>({
      display: 'grid',
      gridTemplateColumns: '1fr 1fr',
      gridTemplateRows: '100px auto',
      gridAutoFlow: 'row',
      gridColumnGap: '10px',
      gridRowGap: '10px',
    });
  });

  it('should support animation properties', () => {
    assertType<LynxCSSProperties>({
      animation: 'fadeIn 1s ease-in-out',
      animationName: 'fadeIn',
      animationDuration: '1s',
      animationTimingFunction: 'ease-in',
      animationDirection: 'alternate',
      animationFillMode: 'forwards',
      animationPlayState: 'running',
    });
  });

  it('should support transition properties', () => {
    assertType<LynxCSSProperties>({
      transition: 'all 0.3s ease',
      transitionProperty: 'opacity',
      transitionDuration: '0.3s',
      transitionTimingFunction: 'cubic-bezier',
    });
  });

  it('should work with strict longhand and shorthand properties', () => {
    assertType<LynxCSSPropertiesWithLonghands>({
      display: 'flex',
      flexDirection: 'row',
      paddingLeft: '10px',
      marginTop: '20px',
    });

    assertType<LynxCSSPropertiesWithShorthands>({
      flex: '1',
      padding: '10px',
      margin: '20px',
      border: '1px solid black',
    });
  });

  it('strict longhands and shorthands should be assignable to LynxCSSProperties', () => {
    assertType<LynxCSSProperties>(lynxCssWithLonghands);
    assertType<LynxCSSProperties>(lynxCssWithShorthands);

    assertType<LynxCSSPropertiesWithLonghands>(lynxCss);
    assertType<LynxCSSPropertiesWithShorthands>(lynxCss);
  });
});

describe('Gradual Migration Type Test', () => {
  it('should support type-safe style objects that work with both types', () => {
    // Users can write styles that satisfy both types by using Lynx-supported values
    const compatibleStyle = {
      display: 'flex' as const,
      flexDirection: 'row' as const,
      justifyContent: 'center' as const,
      alignItems: 'center' as const,
      padding: '10px',
    };

    // Works with loose CSSProperties
    assertType<CSSProperties>(compatibleStyle);

    // Works with strict LynxCSSProperties
    assertType<LynxCSSProperties>(compatibleStyle);
  });

  it('CSSProperties accepts properties not in LynxCSSProperties', () => {
    // This is the key benefit of loose mode - web compatibility
    assertType<CSSProperties>({
      textTransform: 'uppercase',
      letterSpacing: '0.1em',
      lineHeight: 1.5,
    });
  });

  it('LynxCSSProperties rejects unsupported properties', () => {
    // This documents that strict mode catches unsupported properties
    assertType<LynxCSSProperties>({
      display: 'flex',
      // @ts-expect-error: textTransform is not in Lynx's css_defines
      textTransform: 'uppercase',
    });
  });
});
